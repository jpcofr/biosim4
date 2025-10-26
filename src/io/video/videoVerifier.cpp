/**
 * @file videoVerifier.cpp
 * @brief Implementation of video verification utilities
 */

#include "videoVerifier.h"

#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

namespace BioSim {
inline namespace v1 {
namespace IO {
namespace Video {

VideoVerificationResult VideoVerifier::verify(const std::string& outputDir, int expectedGenerations, bool verbose) {
  VideoVerificationResult result;
  result.expectedCount = expectedGenerations;
  result.foundVideos = listVideos(outputDir);
  result.actualCount = static_cast<int>(result.foundVideos.size());

  // Check which generations are missing
  std::vector<bool> found(expectedGenerations, false);
  for (const auto& video : result.foundVideos) {
    if (video.generationNumber >= 0 && video.generationNumber < expectedGenerations) {
      found[video.generationNumber] = true;
    }
  }

  for (int i = 0; i < expectedGenerations; ++i) {
    if (!found[i]) {
      result.missingGenerations.push_back(i);
    }
  }

  result.success = result.missingGenerations.empty() && result.actualCount > 0;

  // Build summary
  std::ostringstream summary;
  if (result.success) {
    summary << "✅ All " << expectedGenerations << " videos generated successfully in " << outputDir;
  } else if (result.actualCount == 0) {
    summary << "❌ No videos found in " << outputDir;
  } else {
    summary << "⚠️  Found " << result.actualCount << "/" << expectedGenerations << " videos in " << outputDir
            << ". Missing: ";
    for (size_t i = 0; i < result.missingGenerations.size(); ++i) {
      if (i > 0)
        summary << ", ";
      summary << result.missingGenerations[i];
    }
  }
  result.summary = summary.str();

  if (verbose) {
    printReport(result);
  }

  return result;
}

std::vector<VideoInfo> VideoVerifier::listVideos(const std::string& outputDir) {
  std::vector<VideoInfo> videos;

  if (!std::filesystem::exists(outputDir)) {
    return videos;
  }

  for (const auto& entry : std::filesystem::directory_iterator(outputDir)) {
    if (entry.is_regular_file()) {
      std::string ext = entry.path().extension().string();
      std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

      if (ext == ".avi" || ext == ".mp4" || ext == ".mov") {
        videos.push_back(getVideoInfo(entry.path()));
      }
    }
  }

  // Sort by generation number
  std::sort(videos.begin(), videos.end(),
            [](const VideoInfo& a, const VideoInfo& b) { return a.generationNumber < b.generationNumber; });

  return videos;
}

VideoInfo VideoVerifier::getVideoInfo(const std::filesystem::path& videoPath) {
  VideoInfo info;
  info.path = videoPath;
  info.exists = std::filesystem::exists(videoPath);

  if (info.exists) {
    info.fileSizeBytes = std::filesystem::file_size(videoPath);
    info.formattedSize = formatFileSize(info.fileSizeBytes);
  } else {
    info.fileSizeBytes = 0;
    info.formattedSize = "N/A";
  }

  info.generationNumber = extractGenerationNumber(videoPath);

  return info;
}

std::string VideoVerifier::formatFileSize(size_t bytes) {
  const char* units[] = {"B", "KB", "MB", "GB"};
  int unit = 0;
  double size = static_cast<double>(bytes);

  while (size >= 1024.0 && unit < 3) {
    size /= 1024.0;
    unit++;
  }

  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << size << " " << units[unit];
  return oss.str();
}

int VideoVerifier::extractGenerationNumber(const std::filesystem::path& path) {
  // Match patterns like: gen-000042.avi, generation_042.mp4, etc.
  std::regex genRegex(R"(gen(?:eration)?[-_]?(\d+))", std::regex::icase);
  std::smatch match;
  std::string filename = path.stem().string();

  if (std::regex_search(filename, match, genRegex) && match.size() > 1) {
    try {
      return std::stoi(match[1].str());
    } catch (...) {
      return -1;
    }
  }

  return -1;
}

void VideoVerifier::printReport(const VideoVerificationResult& result) {
  fmt::print("\n╔══════════════════════════════════════════╗\n");
  fmt::print("║      Video Generation Report             ║\n");
  fmt::print("╚══════════════════════════════════════════╝\n\n");

  fmt::print("{}\n\n", result.summary);

  if (!result.foundVideos.empty()) {
    fmt::print("📹 Found Videos:\n");
    fmt::print("┌────────────┬──────────────────────────────────────────┬────────────┐\n");
    fmt::print("│ Generation │ Filename                                 │ Size       │\n");
    fmt::print("├────────────┼──────────────────────────────────────────┼────────────┤\n");

    for (const auto& video : result.foundVideos) {
      fmt::print("│ {:>10} │ {:<40} │ {:>10} │\n",
                 (video.generationNumber >= 0 ? std::to_string(video.generationNumber) : "unknown"),
                 video.path.filename().string(), video.formattedSize);
    }

    fmt::print("└────────────┴──────────────────────────────────────────┴────────────┘\n");
  }

  if (!result.missingGenerations.empty()) {
    fmt::print("\n❌ Missing generations: ");
    for (size_t i = 0; i < result.missingGenerations.size(); ++i) {
      if (i > 0)
        fmt::print(", ");
      fmt::print("{}", result.missingGenerations[i]);
    }
    fmt::print("\n");
  }

  fmt::print("\n");
}

bool VideoVerifier::openVideoInPlayer(const std::filesystem::path& videoPath) {
  if (!std::filesystem::exists(videoPath)) {
    fmt::print(stderr, "❌ Video not found: {}\n", videoPath.string());
    return false;
  }

#ifdef __APPLE__
  std::string command = "open \"" + videoPath.string() + "\"";
#elif defined(_WIN32)
  std::string command = "start \"\" \"" + videoPath.string() + "\"";
#else
  std::string command = "xdg-open \"" + videoPath.string() + "\"";
#endif

  int result = std::system(command.c_str());
  if (result == 0) {
    fmt::print("🎬 Opened: {}\n", videoPath.filename().string());
    return true;
  } else {
    fmt::print(stderr, "❌ Failed to open video player\n");
    return false;
  }
}

void VideoVerifier::interactiveReview(const std::string& outputDir) {
  auto videos = listVideos(outputDir);

  if (videos.empty()) {
    fmt::print("❌ No videos found in {}\n", outputDir);
    return;
  }

  fmt::print("\n╔══════════════════════════════════════════╗\n");
  fmt::print("║     Interactive Video Review             ║\n");
  fmt::print("╚══════════════════════════════════════════╝\n\n");

  fmt::print("Found {} video(s)\n\n", videos.size());

  for (size_t i = 0; i < videos.size(); ++i) {
    const auto& video = videos[i];
    fmt::print("[{}] Generation {} - {} ({})\n", (i + 1), video.generationNumber, video.path.filename().string(),
               video.formattedSize);
  }

  fmt::print("\nCommands:\n");
  fmt::print("  1-{}  : Open video in player\n", videos.size());
  fmt::print("  a     : Open all videos\n");
  fmt::print("  q     : Quit\n");
  fmt::print("\n");

  std::string input;
  while (true) {
    fmt::print("Choice > ");
    std::getline(std::cin, input);

    if (input.empty())
      continue;

    if (input == "q" || input == "quit" || input == "exit") {
      fmt::print("👋 Goodbye!\n");
      break;
    }

    if (input == "a" || input == "all") {
      fmt::print("🎬 Opening all videos...\n");
      for (const auto& video : videos) {
        openVideoInPlayer(video.path);
      }
      continue;
    }

    try {
      int choice = std::stoi(input);
      if (choice >= 1 && choice <= static_cast<int>(videos.size())) {
        openVideoInPlayer(videos[choice - 1].path);
      } else {
        fmt::print("❌ Invalid choice. Enter 1-{}\n", videos.size());
      }
    } catch (...) {
      fmt::print("❌ Invalid input. Try again.\n");
    }
  }
}

}  // namespace Video
}  // namespace IO
}  // namespace v1
}  // namespace BioSim
