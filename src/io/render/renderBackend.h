#ifndef BIOSIM_RENDER_BACKEND_H_INCLUDED
#define BIOSIM_RENDER_BACKEND_H_INCLUDED

/**
 * @file renderBackend.h
 * @brief Abstract rendering backend interface for visualization output.
 *
 * This header defines a pure virtual interface that isolates the simulation's
 * visualization code from specific graphics libraries (CImg/OpenCV/raylib).
 * This allows swapping rendering implementations without changing the core
 * simulation or ImageWriter logic.
 *
 * Design Goals:
 * - Decouple simulation visualization from specific graphics libraries
 * - Enable testing with mock implementations
 * - Facilitate migration from CImg/OpenCV to raylib
 * - Maintain performance (minimize copy overhead)
 *
 * @note All coordinates use simulation grid coordinates, not pixel coordinates.
 *       The backend is responsible for scaling to display pixels.
 */

#include "../../types/basicTypes.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace BioSim {
inline namespace v1 {
namespace IO {
namespace Render {

/**
 * @struct Color
 * @brief RGB color with optional alpha channel for rendering operations.
 *
 * Color values are normalized to 0-255 range. Alpha is used for transparency
 * effects in pheromone rendering (0=transparent, 255=opaque).
 */
struct Color {
  uint8_t r;  ///< Red component (0-255)
  uint8_t g;  ///< Green component (0-255)
  uint8_t b;  ///< Blue component (0-255)
  uint8_t a;  ///< Alpha/opacity (0=transparent, 255=opaque)

  Color() : r(0), g(0), b(0), a(255) {}
  Color(uint8_t red, uint8_t green, uint8_t blue, uint8_t alpha = 255) : r(red), g(green), b(blue), a(alpha) {}

  /**
   * @brief Create a color from normalized float values (0.0-1.0 range).
   */
  static Color fromFloat(float red, float green, float blue, float alpha = 1.0f) {
    return Color(static_cast<uint8_t>(red * 255.0f), static_cast<uint8_t>(green * 255.0f),
                 static_cast<uint8_t>(blue * 255.0f), static_cast<uint8_t>(alpha * 255.0f));
  }
};

/**
 * @enum ChallengeZoneType
 * @brief Visual indicators for different survival challenge zones.
 *
 * These correspond to the challenge parameter in biosim4.ini and determine
 * what overlay graphics are drawn to show safe/unsafe zones.
 */
enum class ChallengeZoneType {
  NONE = 0,               ///< No visual challenge zone
  CENTER_WEIGHTED = 1,    ///< Green circle in center (weighted survival)
  CENTER_UNWEIGHTED = 2,  ///< Green circle in center (binary survival)
  RADIOACTIVE_WALLS = 3   ///< Yellow wall on left or right side
};

/**
 * @class IRenderBackend
 * @brief Abstract interface for rendering simulation frames to images/video.
 *
 * This interface defines all operations needed to visualize a simulation frame:
 * creating blank canvases, drawing primitives (circles, rectangles), and saving
 * to video files. Concrete implementations wrap specific libraries (CImg, raylib).
 *
 * Coordinate System:
 * - Input coordinates are in simulation grid space (0 to gridSize_X/Y)
 * - Backend scales to pixel space (gridCoord * displayScale)
 * - Y-axis may need flipping depending on backend (CImg uses inverted Y)
 *
 * Thread Safety:
 * - Methods are NOT thread-safe by default
 * - Caller must ensure serial access or use synchronization
 *
 * Lifetime:
 * - Create once, reuse for entire simulation run
 * - Call beginFrame() before drawing, endFrame() when complete
 * - Call saveVideo() at generation boundaries
 */
class IRenderBackend {
 public:
  virtual ~IRenderBackend() = default;

  /**
   * @brief Initialize the rendering backend with simulation parameters.
   *
   * Must be called once before any rendering operations. Sets up internal
   * buffers and state for the given grid dimensions and scaling factor.
   *
   * @param gridWidth Simulation grid width in cells
   * @param gridHeight Simulation grid height in cells
   * @param displayScale Pixel scaling factor (pixels per grid cell)
   * @param agentSize Radius of agent circles in pixels
   */
  virtual void init(uint16_t gridWidth, uint16_t gridHeight, uint16_t displayScale, uint16_t agentSize) = 0;

  /**
   * @brief Start a new video generation, clearing any accumulated frames.
   *
   * Called at the beginning of each generation to reset frame buffers.
   * Any previously buffered frames are discarded.
   */
  virtual void startNewGeneration() = 0;

  /**
   * @brief Begin a new frame, initializing canvas with white background.
   *
   * Creates a blank white image canvas at the configured resolution
   * (gridWidth * displayScale x gridHeight * displayScale pixels).
   * All subsequent draw calls affect this canvas until endFrame() is called.
   *
   * @param simStep Current simulation step number (for debugging/metadata)
   * @param generation Current generation number (for debugging/metadata)
   */
  virtual void beginFrame(unsigned simStep, unsigned generation) = 0;

  /**
   * @brief Draw a challenge zone overlay (safe/unsafe regions).
   *
   * Renders visual indicators for survival challenges: green circles for
   * center challenges, yellow walls for radioactive wall challenges.
   *
   * @param zoneType Type of challenge zone to render
   * @param simStep Current simulation step (used for radioactive wall animation)
   * @param stepsPerGeneration Total steps per generation (for animation timing)
   */
  virtual void drawChallengeZone(ChallengeZoneType zoneType, unsigned simStep, unsigned stepsPerGeneration) = 0;

  /**
   * @brief Draw a filled rectangle with color and transparency.
   *
   * Coordinates are in grid space (not pixels). The backend handles scaling
   * and coordinate system transformations.
   *
   * @param x1 Grid X coordinate of top-left corner
   * @param y1 Grid Y coordinate of top-left corner
   * @param x2 Grid X coordinate of bottom-right corner
   * @param y2 Grid Y coordinate of bottom-right corner
   * @param color Fill color with alpha channel
   *
   * @note Y-coordinates follow simulation convention (0=bottom, max=top)
   * @note Backend must handle Y-axis inversion if needed for display
   */
  virtual void drawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const Color& color) = 0;

  /**
   * @brief Draw a filled circle at a grid location.
   *
   * Coordinates are in grid space. The center coordinate and radius are
   * scaled to pixels by the backend.
   *
   * @param centerX Grid X coordinate of circle center
   * @param centerY Grid Y coordinate of circle center
   * @param radius Circle radius in pixels (NOT grid units)
   * @param color Fill color with alpha channel
   *
   * @note Radius is in pixel space, not grid space (unlike centerX/Y)
   * @note Y-coordinate follows simulation convention (0=bottom, max=top)
   */
  virtual void drawCircle(int16_t centerX, int16_t centerY, uint16_t radius, const Color& color) = 0;

  /**
   * @brief Finalize the current frame and buffer it for video output.
   *
   * Completes the frame started by beginFrame() and adds it to the internal
   * video frame buffer. The frame can be encoded to video later via saveVideo().
   */
  virtual void endFrame() = 0;

  /**
   * @brief Encode all buffered frames into a video file.
   *
   * Writes accumulated frames from the current generation to an AVI video file
   * with the specified generation number in the filename. Clears frame buffer
   * after successful encoding.
   *
   * Video properties (implementation-dependent):
   * - Format: AVI container
   * - Codec: H264 preferred, MJPEG fallback
   * - Frame rate: 25 FPS
   * - Resolution: gridWidth * displayScale x gridHeight * displayScale
   *
   * @param generation Generation number (used for filename: gen-NNNNNN.avi)
   * @param outputPath Base directory path for video output (e.g., "output/images/")
   * @return true if video saved successfully, false on error
   *
   * @note Blocking operation - may take several seconds for long videos
   * @note Logs errors to stderr but does not throw exceptions
   */
  virtual bool saveVideo(unsigned generation, const std::string& outputPath) = 0;

  /**
   * @brief Get the count of video frames currently buffered.
   *
   * Returns the number of frames accumulated since startNewGeneration().
   * Useful for logging and debugging video generation issues.
   *
   * @return Number of buffered frames waiting to be encoded
   */
  virtual size_t getFrameCount() const = 0;
};

/**
 * @brief Factory function to create the default rendering backend.
 *
 * Currently returns a CImg-based implementation. In the future, this can
 * be extended to return different backends based on compile-time or
 * runtime configuration (e.g., -DUSE_RAYLIB flag).
 *
 * @return Unique pointer to a concrete IRenderBackend implementation
 *
 * @note Caller takes ownership of the returned backend
 * @note Backend must be initialized via init() before use
 */
std::unique_ptr<IRenderBackend> createDefaultRenderBackend();

}  // namespace Render
}  // namespace IO
}  // namespace v1
}  // namespace BioSim

// Backward compatibility aliases
namespace BioSim {
using IO::Render::ChallengeZoneType;
using IO::Render::Color;
using IO::Render::createDefaultRenderBackend;
using IO::Render::IRenderBackend;
}  // namespace BioSim

#endif  // BIOSIM_RENDER_BACKEND_H_INCLUDED
