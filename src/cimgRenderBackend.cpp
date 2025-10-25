/**
 * @file cimgRenderBackend.cpp
 * @brief CImg-based implementation of the rendering backend interface.
 *
 * This file provides a concrete implementation of IRenderBackend using the CImg
 * library with OpenCV backend for video encoding. This implementation matches
 * the current behavior of imageWriter.cpp, allowing us to maintain compatibility
 * while isolating the library-specific code.
 *
 * This implementation will eventually be replaced with a raylib-based backend,
 * but serves as the bridge during the refactoring process.
 */

#include "params.h"
#include "renderBackend.h"

#include <iomanip>
#include <iostream>
#include <sstream>

/// Enable OpenCV support in CImg for video encoding
#define cimg_use_opencv 1
#define cimg_display 0
#include "CImg.h"

namespace BioSim {

/**
 * @class CImgRenderBackend
 * @brief Concrete rendering backend using CImg library with OpenCV video support.
 *
 * This class wraps CImg drawing operations and OpenCV video encoding in the
 * IRenderBackend interface. It maintains a CImgList to buffer frames and
 * provides drawing primitives that match the original imageWriter.cpp behavior.
 *
 * Implementation Details:
 * - Uses CImg<uint8_t> for RGB images (3 color channels)
 * - Y-axis is inverted (CImg convention: 0=top, max=bottom)
 * - Alpha blending done via CImg's draw_* methods
 * - Video encoding via CImg::save_video() (uses OpenCV backend)
 *
 * Thread Safety:
 * - NOT thread-safe - caller must synchronize access
 * - Single-threaded use intended (sync mode in ImageWriter)
 */
class CImgRenderBackend : public IRenderBackend {
 public:
  CImgRenderBackend();
  ~CImgRenderBackend() override = default;

  void init(uint16_t gridWidth, uint16_t gridHeight, uint16_t displayScale, uint16_t agentSize) override;
  void startNewGeneration() override;
  void beginFrame(unsigned simStep, unsigned generation) override;
  void drawChallengeZone(ChallengeZoneType zoneType, unsigned simStep, unsigned stepsPerGeneration) override;
  void drawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const Color& color) override;
  void drawCircle(int16_t centerX, int16_t centerY, uint16_t radius, const Color& color) override;
  void endFrame() override;
  bool saveVideo(unsigned generation, const std::string& outputPath) override;
  size_t getFrameCount() const override;

 private:
  /**
   * @brief Convert simulation Y coordinate to CImg Y coordinate (inverted).
   *
   * CImg uses top-left origin (Y increases downward), but simulation uses
   * bottom-left origin (Y increases upward). This helper inverts the Y axis.
   *
   * @param simY Simulation Y coordinate (0 = bottom of grid)
   * @return CImg Y coordinate (0 = top of image)
   */
  int16_t toCImgY(int16_t simY) const;

  uint16_t gridWidth_;     ///< Simulation grid width in cells
  uint16_t gridHeight_;    ///< Simulation grid height in cells
  uint16_t displayScale_;  ///< Pixel scaling factor (pixels per grid cell)
  uint16_t agentSize_;     ///< Agent circle radius in pixels

  cimg_library::CImg<uint8_t> currentFrame_;     ///< Frame currently being drawn
  cimg_library::CImgList<uint8_t> frameBuffer_;  ///< Accumulated frames for video
  bool frameInProgress_;                         ///< True if beginFrame() called without endFrame()
};

CImgRenderBackend::CImgRenderBackend()
    : gridWidth_(0), gridHeight_(0), displayScale_(1), agentSize_(1), frameInProgress_(false) {}

void CImgRenderBackend::init(uint16_t gridWidth, uint16_t gridHeight, uint16_t displayScale, uint16_t agentSize) {
  gridWidth_ = gridWidth;
  gridHeight_ = gridHeight;
  displayScale_ = displayScale;
  agentSize_ = agentSize;
  startNewGeneration();
}

void CImgRenderBackend::startNewGeneration() {
  frameBuffer_.clear();
  frameInProgress_ = false;
}

void CImgRenderBackend::beginFrame(unsigned simStep, unsigned generation) {
  using namespace cimg_library;

  if (frameInProgress_) {
    std::cerr << "Warning: beginFrame() called while frame in progress. Discarding previous frame." << std::endl;
  }

  // Create white background canvas (3 channels RGB, initialized to 255)
  currentFrame_ = CImg<uint8_t>(gridWidth_ * displayScale_, gridHeight_ * displayScale_,
                                1,     // Z depth
                                3,     // RGB channels
                                255);  // White background
  frameInProgress_ = true;
}

void CImgRenderBackend::drawChallengeZone(ChallengeZoneType zoneType, unsigned simStep, unsigned stepsPerGeneration) {
  if (!frameInProgress_) {
    std::cerr << "Error: drawChallengeZone() called without beginFrame()" << std::endl;
    return;
  }

  uint8_t color[3];
  uint16_t offset;

  switch (zoneType) {
    case ChallengeZoneType::CENTER_WEIGHTED:
    case ChallengeZoneType::CENTER_UNWEIGHTED:
      // Green circle in center
      color[0] = 0xa0;
      color[1] = 0xff;
      color[2] = 0xa0;
      currentFrame_.draw_circle((gridWidth_ * displayScale_) / 2, (gridHeight_ * displayScale_) / 2,
                                (gridHeight_ / 3.0 * displayScale_), color, 1.0);
      break;

    case ChallengeZoneType::RADIOACTIVE_WALLS:
      // Yellow wall on left or right (switches at halfway point)
      color[0] = 0xff;
      color[1] = 0xff;
      color[2] = 0xa0;
      offset = 0;
      if (simStep >= stepsPerGeneration / 2) {
        offset = gridWidth_ - 5;
      }
      currentFrame_.draw_rectangle(offset * displayScale_, 0, (offset + 5) * displayScale_, gridHeight_ * displayScale_,
                                   color, 1.0);
      break;

    case ChallengeZoneType::NONE:
    default:
      // No challenge zone to draw
      break;
  }
}

void CImgRenderBackend::drawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const Color& color) {
  if (!frameInProgress_) {
    std::cerr << "Error: drawRectangle() called without beginFrame()" << std::endl;
    return;
  }

  // Convert simulation coordinates to pixel coordinates
  int16_t pixelX1 = x1 * displayScale_;
  int16_t pixelY1 = toCImgY(y1) * displayScale_;
  int16_t pixelX2 = x2 * displayScale_;
  int16_t pixelY2 = toCImgY(y2) * displayScale_;

  uint8_t cimgColor[3] = {color.r, color.g, color.b};
  float alpha = color.a / 255.0f;

  currentFrame_.draw_rectangle(pixelX1, pixelY1, pixelX2, pixelY2, cimgColor, alpha);
}

void CImgRenderBackend::drawCircle(int16_t centerX, int16_t centerY, uint16_t radius, const Color& color) {
  if (!frameInProgress_) {
    std::cerr << "Error: drawCircle() called without beginFrame()" << std::endl;
    return;
  }

  // Convert simulation coordinates to pixel coordinates
  int16_t pixelX = centerX * displayScale_;
  int16_t pixelY = toCImgY(centerY) * displayScale_;

  uint8_t cimgColor[3] = {color.r, color.g, color.b};
  float alpha = color.a / 255.0f;

  currentFrame_.draw_circle(pixelX, pixelY, radius, cimgColor, alpha);
}

void CImgRenderBackend::endFrame() {
  if (!frameInProgress_) {
    std::cerr << "Error: endFrame() called without beginFrame()" << std::endl;
    return;
  }

  frameBuffer_.push_back(currentFrame_);
  frameInProgress_ = false;
}

bool CImgRenderBackend::saveVideo(unsigned generation, const std::string& outputPath) {
  if (frameBuffer_.size() == 0) {
    std::cerr << "Warning: No frames to save for generation " << generation << std::endl;
    return false;
  }

  std::stringstream videoFilename;
  std::string imgDir = outputPath;

  // Add trailing slash if not present
  if (!imgDir.empty() && imgDir.back() != '/') {
    imgDir += '/';
  }

  videoFilename << imgDir << "gen-" << std::setfill('0') << std::setw(6) << generation << ".avi";

  std::cout << "Saving " << frameBuffer_.size() << " frames to " << videoFilename.str() << std::endl;

  try {
    cv::setNumThreads(2);
    frameBuffer_.save_video(videoFilename.str().c_str(), 25, "H264");
    std::cout << "Video saved successfully" << std::endl;
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Error saving video with H264: " << e.what() << std::endl;
    std::cerr << "Trying MJPEG codec..." << std::endl;
    try {
      frameBuffer_.save_video(videoFilename.str().c_str(), 25, "MJPG");
      std::cout << "Video saved with MJPEG codec" << std::endl;
      return true;
    } catch (const std::exception& e2) {
      std::cerr << "Video encoding failed: " << e2.what() << std::endl;
      return false;
    }
  }
}

size_t CImgRenderBackend::getFrameCount() const {
  return frameBuffer_.size();
}

int16_t CImgRenderBackend::toCImgY(int16_t simY) const {
  // Invert Y axis: simulation Y=0 is bottom, CImg Y=0 is top
  return gridHeight_ - simY - 1;
}

// Factory function implementation
std::unique_ptr<IRenderBackend> createDefaultRenderBackend() {
  return std::make_unique<CImgRenderBackend>();
}

}  // namespace BioSim
