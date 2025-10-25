/**
 * @file videoVerifier.cpp
 * @brief Implementation of video verification utilities
 */

#include "videoVerifier.h"

#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <regex>
#include <sstream>

namespace BioSim {

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
    summary << "✅ All " << expectedGenerations << " videos generated successfully!";
  } else if (result.actualCount == 0) {
    summary << "❌ No videos found in " << outputDir;
  } else {
    summary << "⚠️  Found " << result.actualCount << "/" << expectedGenerations << " videos. Missing: ";
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
  std::cout << "\n╔══════════════════════════════════════════╗\n";
  std::cout << "║      Video Generation Report             ║\n";
  std::cout << "╚══════════════════════════════════════════╝\n\n";

  std::cout << result.summary << "\n\n";

  if (!result.foundVideos.empty()) {
    std::cout << "📹 Found Videos:\n";
    std::cout << "┌────────────┬──────────────────────────────────────────┬────────────┐\n";
    std::cout << "│ Generation │ Filename                                 │ Size       │\n";
    std::cout << "├────────────┼──────────────────────────────────────────┼────────────┤\n";

    for (const auto& video : result.foundVideos) {
      std::cout << "│ " << std::setw(10)
                << (video.generationNumber >= 0 ? std::to_string(video.generationNumber) : "unknown") << " │ "
                << std::setw(40) << std::left << video.path.filename().string() << " │ " << std::setw(10) << std::right
                << video.formattedSize << " │\n";
    }

    std::cout << "└────────────┴──────────────────────────────────────────┴────────────┘\n";
  }

  if (!result.missingGenerations.empty()) {
    std::cout << "\n❌ Missing generations: ";
    for (size_t i = 0; i < result.missingGenerations.size(); ++i) {
      if (i > 0)
        std::cout << ", ";
      std::cout << result.missingGenerations[i];
    }
    std::cout << "\n";
  }

  std::cout << "\n";
}

bool VideoVerifier::openVideoInPlayer(const std::filesystem::path& videoPath) {
  if (!std::filesystem::exists(videoPath)) {
    std::cerr << "❌ Video not found: " << videoPath << std::endl;
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
    std::cout << "🎬 Opened: " << videoPath.filename() << std::endl;
    return true;
  } else {
    std::cerr << "❌ Failed to open video player" << std::endl;
    return false;
  }
}

void VideoVerifier::interactiveReview(const std::string& outputDir) {
  auto videos = listVideos(outputDir);

  if (videos.empty()) {
    std::cout << "❌ No videos found in " << outputDir << std::endl;
    return;
  }

  std::cout << "\n╔══════════════════════════════════════════╗\n";
  std::cout << "║     Interactive Video Review             ║\n";
  std::cout << "╚══════════════════════════════════════════╝\n\n";

  std::cout << "Found " << videos.size() << " video(s)\n\n";

  for (size_t i = 0; i < videos.size(); ++i) {
    const auto& video = videos[i];
    std::cout << "[" << (i + 1) << "] Generation " << video.generationNumber << " - " << video.path.filename().string()
              << " (" << video.formattedSize << ")\n";
  }

  std::cout << "\nCommands:\n";
  std::cout << "  1-" << videos.size() << "  : Open video in player\n";
  std::cout << "  a     : Open all videos\n";
  std::cout << "  q     : Quit\n";
  std::cout << "\n";

  std::string input;
  while (true) {
    std::cout << "Choice > ";
    std::getline(std::cin, input);

    if (input.empty())
      continue;

    if (input == "q" || input == "quit" || input == "exit") {
      std::cout << "👋 Goodbye!\n";
      break;
    }

    if (input == "a" || input == "all") {
      std::cout << "🎬 Opening all videos...\n";
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
        std::cout << "❌ Invalid choice. Enter 1-" << videos.size() << "\n";
      }
    } catch (...) {
      std::cout << "❌ Invalid input. Try again.\n";
    }
  }
}

}  // namespace BioSim
