/**
 * @file videoVerifier.h
 * @brief Interactive video generation verification tool
 *
 * This utility helps verify that video generation is working correctly,
 * especially during transitions (like removing CImg/OpenCV dependencies).
 * It checks for generated videos, validates their properties, and provides
 * helpful feedback.
 */

#ifndef VIDEOVERIFIER_H
#define VIDEOVERIFIER_H

#include <filesystem>
#include <string>
#include <vector>

namespace BioSim {
inline namespace v1 {
namespace IO {
namespace Video {

/**
 * @brief Video file information
 */
struct VideoInfo {
  std::filesystem::path path;
  size_t fileSizeBytes;
  std::string formattedSize;
  int generationNumber;
  bool exists;
};

/**
 * @brief Video verification results
 */
struct VideoVerificationResult {
  bool success;
  int expectedCount;
  int actualCount;
  std::vector<VideoInfo> foundVideos;
  std::vector<int> missingGenerations;
  std::string summary;
};

/**
 * @brief Utility for verifying video generation output
 */
class VideoVerifier {
 public:
  /**
   * @brief Verify videos were generated as expected
   *
   * @param outputDir Directory where videos should be located
   * @param expectedGenerations Number of generation videos expected
   * @param verbose Print detailed information
   * @return Verification results
   */
  static VideoVerificationResult verify(const std::string& outputDir, int expectedGenerations, bool verbose = true);

  /**
   * @brief List all video files in output directory
   *
   * @param outputDir Directory to search
   * @return List of found videos with metadata
   */
  static std::vector<VideoInfo> listVideos(const std::string& outputDir);

  /**
   * @brief Get information about a specific video file
   *
   * @param videoPath Path to video file
   * @return Video information
   */
  static VideoInfo getVideoInfo(const std::filesystem::path& videoPath);

  /**
   * @brief Print formatted verification report
   *
   * @param result Verification results to print
   */
  static void printReport(const VideoVerificationResult& result);

  /**
   * @brief Open video in system default player (macOS)
   *
   * @param videoPath Path to video file
   * @return true if video was opened successfully
   */
  static bool openVideoInPlayer(const std::filesystem::path& videoPath);

  /**
   * @brief Interactive video review mode
   *
   * Allows user to browse and preview generated videos.
   *
   * @param outputDir Directory containing videos
   */
  static void interactiveReview(const std::string& outputDir);

 private:
  static std::string formatFileSize(size_t bytes);
  static int extractGenerationNumber(const std::filesystem::path& path);
};

}  // namespace Video
}  // namespace IO
}  // namespace v1
}  // namespace BioSim

// Backward compatibility aliases
namespace BioSim {
using IO::Video::VideoInfo;
using IO::Video::VideoVerificationResult;
using IO::Video::VideoVerifier;
}  // namespace BioSim

#endif  // VIDEOVERIFIER_H
