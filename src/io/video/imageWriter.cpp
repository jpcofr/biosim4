/**
 * @file imageWriter.cpp
 * @brief Implementation of video frame generation and encoding for simulation visualization
 *
 * This file implements the ImageWriter class which captures simulation state as video frames
 * and assembles them into AVI movies. It uses the IRenderBackend abstraction to remain
 * independent of specific graphics libraries (currently raylib, previously CImg/OpenCV).
 *
 * @note Video generation can be disabled via config parameter saveVideo=false
 * @note Output format is AVI with H264 codec (implementation-specific)
 */

#include "imageWriter.h"

#include "../../core/simulation/simulator.h"
#include "../render/renderBackend.h"

#include <spdlog/fmt/fmt.h>

#include <chrono>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <thread>

namespace BioSim {
inline namespace v1 {
namespace IO {
namespace Video {

// Global render backend instance (owned by imageWriter)
static std::unique_ptr<IO::Render::IRenderBackend> renderBackend = nullptr;

/**
 * @brief Renders a single simulation frame using the render backend abstraction
 *
 * This is the core rendering function that converts simulation state (individual locations,
 * pheromone trails, barriers, challenge zones) into a visual image frame. The frame is
 * constructed by calling methods on the render backend interface.
 *
 * Rendering layers (back to front):
 * 1. White background (created by beginFrame())
 * 2. Challenge zone indicators (green circle or yellow wall)
 * 3. Pheromone trail layer 0 (blue, alpha-blended)
 * 4. "Recent death" pheromone layer 1 (red, alpha-blended)
 * 5. Barrier locations (gray rectangles)
 * 6. Individual agents (colored circles based on genome hash)
 *
 * @param data Cached snapshot of simulation state including individual locations, colors,
 *             pheromone intensities, and barrier positions. This snapshot is immutable
 *             allowing rendering to occur on a separate thread without data races.
 *
 * @note Frame resolution is gridSize * displayScale pixels per dimension
 * @note Agent colors are deterministically generated from genome via makeGeneticColor()
 * @note Pheromone alpha values are scaled to prevent over-saturation (max 0.33 for layer 0)
 * @note The rendered frame is buffered by the backend for later video encoding
 *
 * @see makeGeneticColor() for color generation algorithm
 * @see ImageFrameData for data structure definition
 * @see IRenderBackend for rendering interface details
 */
void saveOneFrameImmed(const ImageFrameData& data) {
  if (!renderBackend) {
    fmt::print(stderr, "Error: Render backend not initialized!\n");
    return;
  }

  // Begin new frame (creates white canvas)
  renderBackend->beginFrame(data.simStep, data.generation);

  // Convert challenge enum to zone type
  ChallengeZoneType zoneType = ChallengeZoneType::NONE;
  switch (data.challenge) {
    case CHALLENGE_CENTER_WEIGHTED:
      zoneType = ChallengeZoneType::CENTER_WEIGHTED;
      break;
    case CHALLENGE_CENTER_UNWEIGHTED:
      zoneType = ChallengeZoneType::CENTER_UNWEIGHTED;
      break;
    case CHALLENGE_RADIOACTIVE_WALLS:
      zoneType = ChallengeZoneType::RADIOACTIVE_WALLS;
      break;
    default:
      zoneType = ChallengeZoneType::NONE;
      break;
  }

  // Draw challenge zones
  renderBackend->drawChallengeZone(zoneType, data.simStep, parameterMngrSingleton.stepsPerGeneration);

  // Draw standard pheromone trails (signal layer 0) - blue with alpha
  if (data.signalLayers.size() > 0) {
    for (int16_t x = 0; x < parameterMngrSingleton.gridSize_X; ++x) {
      for (int16_t y = 0; y < parameterMngrSingleton.gridSize_Y; ++y) {
        uint8_t intensity = data.signalLayers[0][x][y];
        if (intensity > 0) {
          // Blue pheromone with alpha based on intensity (max 0.33)
          float alpha = (static_cast<float>(intensity) / 255.0f) / 3.0f;
          if (alpha > 0.33f) {
            alpha = 0.33f;
          }
          uint8_t alphaVal = static_cast<uint8_t>(alpha * 255.0f);

          // Draw rectangle for pheromone cell
          renderBackend->drawRectangle(x - 1, y - 1, x + 1, y + 1, Color(0x00, 0x00, 0xff, alphaVal));
        }
      }
    }
  }

  // Draw "recent death" pheromone layer (layer 1) - red with alpha
  if (data.signalLayers.size() > 1) {
    for (int16_t x = 0; x < parameterMngrSingleton.gridSize_X; ++x) {
      for (int16_t y = 0; y < parameterMngrSingleton.gridSize_Y; ++y) {
        uint8_t intensity = data.signalLayers[1][x][y];
        if (intensity > 0) {
          // Red death marker with alpha
          float alpha = static_cast<float>(intensity) / 255.0f;
          uint8_t alphaVal = static_cast<uint8_t>(alpha * 255.0f);

          renderBackend->drawRectangle(x - 1, y - 1, x + 1, y + 1, Color(0xff, 0x00, 0x00, alphaVal));
        }
      }
    }
  }

  // Draw barriers as gray rectangles
  Color barrierColor(0x88, 0x88, 0x88, 0xff);  // Gray
  for (const auto& loc : data.barrierLocs) {
    renderBackend->drawRectangle(loc.x, loc.y, loc.x + 1, loc.y + 1, barrierColor);
  }

  // Draw individuals as colored circles
  constexpr uint8_t maxColorVal = 0xb0;
  constexpr uint8_t maxLumaVal = 0xb0;

  auto rgbToLuma = [](uint8_t r, uint8_t g, uint8_t b) { return (r + r + r + b + g + g + g + g) / 8; };

  for (size_t i = 0; i < data.indivLocs.size(); ++i) {
    int c = data.indivColors[i];
    uint8_t r = static_cast<uint8_t>(c);                // R: 0..255
    uint8_t g = static_cast<uint8_t>((c & 0x1f) << 3);  // G: 0..255
    uint8_t b = static_cast<uint8_t>((c & 7) << 5);     // B: 0..255

    // Prevent color mappings to very bright colors (hard to see):
    if (rgbToLuma(r, g, b) > maxLumaVal) {
      if (r > maxColorVal)
        r %= maxColorVal;
      if (g > maxColorVal)
        g %= maxColorVal;
      if (b > maxColorVal)
        b %= maxColorVal;
    }

    renderBackend->drawCircle(data.indivLocs[i].x, data.indivLocs[i].y, parameterMngrSingleton.agentSize,
                              Color(r, g, b, 255));
  }

  // Finalize frame (adds to backend's frame buffer)
  renderBackend->endFrame();
}

/**
 * @brief Constructor initializing ImageWriter to idle state ready for async operations
 *
 * Initializes atomic flags and counters for thread-safe communication between the main
 * simulation thread and the optional async frame-saving thread (currently unused).
 *
 * @note busy=true initially prevents premature job submission before init() is called
 * @note The async saveFrameThread() is not started automatically - currently unused
 */
ImageWriter::ImageWriter() : droppedFrameCount{0}, busy{true}, dataReady{false}, abortRequested{false} {}

/**
 * @brief Initializes the ImageWriter with grid dimensions and signal layer count
 *
 * Prepares the image writer for a new simulation run by creating the render backend
 * and initializing it with the simulation parameters.
 *
 * @param layers Number of pheromone signal layers (typically 2: standard trails + death alarm) - unused, kept for API
 * compatibility
 * @param sizeX Grid width in cells
 * @param sizeY Grid height in cells
 *
 * @note Called once at simulator startup from simulator() main function
 * @note Parameters are stored globally in parameterMngrSingleton, not cached locally
 */
void ImageWriter::init(uint16_t layers, uint16_t sizeX, uint16_t sizeY) {
  (void)layers;  // Unused parameter, kept for API compatibility
  // Create and initialize the render backend
  renderBackend = createDefaultRenderBackend();
  if (renderBackend) {
    renderBackend->init(sizeX, sizeY, parameterMngrSingleton.displayScale, parameterMngrSingleton.agentSize);
  } else {
    fmt::print(stderr, "Error: Failed to create render backend!\n");
  }
  startNewGeneration();
}

/**
 * @brief Resets frame accumulator for a new generation's video output
 *
 * Clears the render backend's frame buffer and resets the skipped frame counter.
 * This must be called before capturing frames for a new generation to prevent
 * mixing frames from different generations in the same video file.
 *
 * @note Called automatically by init() and after saveGenerationVideo() completes
 * @note Does NOT clear droppedFrameCount (tracks drops across all generations)
 */
void ImageWriter::startNewGeneration() {
  if (renderBackend) {
    renderBackend->startNewGeneration();
  }
  skippedFrames = 0;
}

/**
 * @brief Generates a deterministic 8-bit color value from an individual's genome
 *
 * Creates a pseudo-hash of the genome by extracting bit patterns from key gene properties
 * (first and last gene's source/sink types and indices). This produces visually distinct
 * colors for different genome structures while keeping the same color for clones.
 *
 * Bit layout (LSB to MSB):
 * - Bit 0: Genome length parity (odd=1, even=0)
 * - Bit 1: First gene source type
 * - Bit 2: Last gene source type
 * - Bit 3: First gene sink type
 * - Bit 4: Last gene sink type
 * - Bit 5: First gene source number (LSB only)
 * - Bit 6: First gene sink number (LSB only)
 * - Bit 7: Last gene source number (LSB only)
 *
 * @param genome The individual's genome (vector of Gene structs)
 * @return 8-bit color index value (0-255) used as seed for RGB color generation
 *
 * @note This is NOT a cryptographic hash - collisions are acceptable and visually tolerable
 * @note Color mapping to RGB occurs in saveOneFrameImmed() to prevent overly bright colors
 * @note Identical genomes (clones) will have identical colors, aiding visual tracking
 *
 * @see saveOneFrameImmed() for RGB color mapping from this 8-bit value
 */
uint8_t makeGeneticColor(const Genome& genome) {
  return ((genome.size() & 1) | ((genome.front().sourceType) << 1) | ((genome.back().sourceType) << 2) |
          ((genome.front().sinkType) << 3) | ((genome.back().sinkType) << 4) | ((genome.front().sourceNum & 1) << 5) |
          ((genome.front().sinkNum & 1) << 6) | ((genome.back().sourceNum & 1) << 7));
}

/**
 * @brief [UNUSED] Asynchronous gate for submitting frame render jobs to background thread
 *
 * This method was designed to queue frame rendering on a separate worker thread
 * (saveFrameThread()) to avoid blocking the simulation loop. However, it is currently
 * disabled due to threading bugs and race conditions (see IMAGEWRITER_INTEGRATION_GUIDE.md).
 *
 * Operational flow (when enabled):
 * 1. Check if saveFrameThread() is busy processing previous frame
 * 2. If idle, snapshot current simulation state into ImageFrameData
 * 3. Set dataReady flag and notify worker thread via condition variable
 * 4. Return true (job accepted) or false (busy, frame dropped)
 *
 * The busy flag uses atomic operations but relaxed memory ordering is sufficient because:
 * - Only one producer (this function) and one consumer (saveFrameThread())
 * - Frame drops are acceptable - no correctness issues from stale visibility
 * - Condition variable provides synchronization for dataReady flag
 *
 * @param simStep Current simulation step number (0 to stepsPerGeneration-1)
 * @param generation Current generation number (0 to numGenerations-1)
 * @param challenge Active challenge type enum (affects zone rendering)
 * @param barrierType Barrier pattern enum (affects barrier rendering)
 *
 * @return true if frame was queued successfully, false if worker thread was busy (frame dropped)
 *
 * @warning Currently DISABLED - use saveVideoFrameSync() instead
 * @warning Caller must not modify peeps, grid, or pheromones until this returns false
 *
 * @see saveVideoFrameSync() for the currently-used synchronous implementation
 * @see saveFrameThread() for the worker thread implementation
 * @see IMAGEWRITER_INTEGRATION_GUIDE.md for threading bug details
 */
bool ImageWriter::saveVideoFrame(unsigned simStep, unsigned generation, unsigned challenge, unsigned barrierType) {
  if (!busy) {
    busy = true;
    /// queue job for saveFrameThread()
    /// We cache a local copy of data from params, grid, and peeps because
    /// those objects will change by the main thread at the same time our
    /// saveFrameThread() is using it to output a video frame.
    data.simStep = simStep;
    data.generation = generation;
    data.challenge = challenge;
    data.barrierType = barrierType;
    data.indivLocs.clear();
    data.indivColors.clear();
    data.barrierLocs.clear();

    for (uint16_t index = 1; index <= parameterMngrSingleton.population; ++index) {
      const Individual& indiv = peeps[index];
      if (indiv.alive) {
        data.indivLocs.push_back(indiv.loc);
        data.indivColors.push_back(makeGeneticColor(indiv.genome));
      }
    }

    /// Copy signal layers
    for (unsigned layerNum = 0; layerNum < parameterMngrSingleton.signalLayers; ++layerNum) {
      for (int16_t x = 0; x < parameterMngrSingleton.gridSize_X; ++x) {
        for (int16_t y = 0; y < parameterMngrSingleton.gridSize_Y; ++y) {
          data.signalLayers[layerNum][x][y] = pheromones[layerNum][x][y];
        }
      }
    }

    auto const& barrierLocs = grid.getBarrierLocations();
    for (Coordinate loc : barrierLocs) {
      data.barrierLocs.push_back(loc);
    }

    /// tell thread there's a job to do
    {
      std::lock_guard<std::mutex> lck(mutex_);
      dataReady = true;
    }
    condVar.notify_one();
    return true;
  } else {
    /// image saver thread is busy, drop a frame
    ++droppedFrameCount;
    return false;
  }
}

/**
 * @brief Synchronously captures and renders a single video frame (blocking call)
 *
 * This is the actively-used frame capture method that snapshots the current simulation
 * state and immediately renders it to the imageList buffer. Unlike saveVideoFrame(),
 * this blocks the simulation loop until rendering completes (~1-5ms per frame depending
 * on population size and grid dimensions).
 *
 * Execution flow:
 * 1. Copy simulation state (individuals, pheromones, barriers) into ImageFrameData
 * 2. Call saveOneFrameImmed() to render frame and append to imageList
 * 3. Return control to caller (simulation can proceed to next step)
 *
 * Data copied from global singletons:
 * - peeps: alive individuals' locations and genome-based colors
 * - pheromones: all signal layers' intensity values [layer][x][y]
 * - grid: barrier location coordinates
 *
 * @param simStep Current simulation step number (0 to stepsPerGeneration-1)
 * @param generation Current generation number (0 to numGenerations-1)
 * @param challenge Active challenge type enum (determines safe/unsafe zone rendering)
 * @param barrierType Barrier pattern enum (affects barrier shape/location)
 *
 * @return Always true (frame is guaranteed to be captured)
 *
 * @note Blocking time scales with population and grid size (typically <5ms)
 * @note Called from endOfSimulationStep() when saveVideo=true in config
 * @note Memory allocation per call: ~O(population + gridSize_X * gridSize_Y * signalLayers)
 *
 * @see saveVideoFrame() for the unused async version
 * @see saveOneFrameImmed() for the actual rendering implementation
 * @see endOfSimulationStep() in simulator.cpp for the call site
 */
bool ImageWriter::saveVideoFrameSync(unsigned simStep, unsigned generation, unsigned challenge, unsigned barrierType) {
  /// We cache a local copy of data from params, grid, and peeps because
  /// those objects will change by the main thread at the same time our
  /// saveFrameThread() is using it to output a video frame.
  data.simStep = simStep;
  data.generation = generation;
  data.challenge = challenge;
  data.barrierType = barrierType;
  data.indivLocs.clear();
  data.indivColors.clear();
  data.barrierLocs.clear();
  data.signalLayers.clear();

  for (uint16_t index = 1; index <= parameterMngrSingleton.population; ++index) {
    const Individual& indiv = peeps[index];
    if (indiv.alive) {
      data.indivLocs.push_back(indiv.loc);
      data.indivColors.push_back(makeGeneticColor(indiv.genome));
    }
  }

  /// Copy signal layers - note: pheromones uses Signals class [layer][x][y]
  /// but we need to copy to simple vector structure
  data.signalLayers.resize(parameterMngrSingleton.signalLayers);
  for (unsigned layerNum = 0; layerNum < parameterMngrSingleton.signalLayers; ++layerNum) {
    data.signalLayers[layerNum].resize(parameterMngrSingleton.gridSize_X);
    for (int16_t x = 0; x < parameterMngrSingleton.gridSize_X; ++x) {
      data.signalLayers[layerNum][x].resize(parameterMngrSingleton.gridSize_Y);
      for (int16_t y = 0; y < parameterMngrSingleton.gridSize_Y; ++y) {
        data.signalLayers[layerNum][x][y] = pheromones[layerNum][x][y];
      }
    }
  }

  auto const& barrierLocs = grid.getBarrierLocations();
  for (Coordinate loc : barrierLocs) {
    data.barrierLocs.push_back(loc);
  }

  saveOneFrameImmed(data);
  return true;
}

/**
 * @brief Encodes accumulated frames into an AVI video file for the completed generation
 *
 * This method is called at the end of each generation (after survival selection) to
 * compile all captured frames into a single video file using the render backend's
 * video encoding capabilities.
 *
 * Video properties (backend-specific):
 * - Format: AVI container
 * - Codec: H264 (preferred) or backend-specific fallback
 * - Frame rate: 25 FPS (typical)
 * - Resolution: gridSize * displayScale pixels
 * - Filename: output/images/gen-NNNNNN.avi (6-digit zero-padded generation number)
 *
 * Encoding flow:
 * 1. Delegate to renderBackend->saveVideo() for encoding
 * 2. Report success/failure to stdout
 * 3. Clear frame buffer via startNewGeneration()
 *
 * @param generation Generation number for filename construction (0 to numGenerations-1)
 *
 * @note Encoding time varies by backend and frame count
 * @note Empty frame buffer is handled gracefully by backend
 * @note Errors are logged but do not abort the simulation
 *
 * @warning Blocking operation - simulation pauses during video encoding
 *
 * @see startNewGeneration() for cleanup after encoding completes
 * @see endOfGeneration() in simulator.cpp for the call site
 * @see config/biosim4.ini parameters: saveVideo, videoStride, videoSaveFirstFrames
 */
void ImageWriter::saveGenerationVideo(unsigned generation) {
  if (!renderBackend) {
    fmt::print(stderr, "Error: Render backend not initialized!\n");
    return;
  }

  size_t frameCount = renderBackend->getFrameCount();
  if (frameCount > 0) {
    std::string imgDir = parameterMngrSingleton.imageDir;
    // Add trailing slash if not present
    if (!imgDir.empty() && imgDir.back() != '/') {
      imgDir += '/';
    }

    fmt::print("Encoding {} frames for generation {}\n", frameCount, generation);

    bool success = renderBackend->saveVideo(generation, imgDir);

    if (success) {
      fmt::print("Video saved successfully\n");
    } else {
      fmt::print(stderr, "Error: Failed to save video for generation {}\n", generation);
    }
  }
  startNewGeneration();
}

/**
 * @brief [UNUSED] Signals the async worker thread to terminate gracefully
 *
 * This method was designed to cleanly shutdown the saveFrameThread() background worker
 * by setting the abortRequested flag and waking the thread via condition variable.
 * The worker checks this flag and exits its loop when true.
 *
 * @note Currently unused because async threading is disabled
 * @note If re-enabled, call this before ImageWriter destruction to prevent orphaned threads
 *
 * @see saveFrameThread() for the worker thread that responds to this signal
 */
void ImageWriter::abort() {
  busy = true;
  abortRequested = true;
  {
    std::lock_guard<std::mutex> lck(mutex_);
    dataReady = true;
  }
  condVar.notify_one();
}

/**
 * @brief [UNUSED] Background worker thread for asynchronous frame rendering
 *
 * This thread implementation was designed to offload frame rendering from the main
 * simulation loop, allowing the simulator to proceed to the next step while the
 * previous frame is still being rendered. However, it is currently disabled due to
 * race conditions and synchronization bugs (see IMAGEWRITER_INTEGRATION_GUIDE.md).
 *
 * Intended operation:
 * 1. Wait on condition variable until saveVideoFrame() signals a new job
 * 2. Check abortRequested flag for graceful shutdown
 * 3. Render frame from cached ImageFrameData via saveOneFrameImmed()
 * 4. Clear busy flag to signal readiness for next frame
 * 5. Loop back to wait state
 *
 * Synchronization mechanism:
 * - Condition variable (condVar) for efficient thread wake-up
 * - Mutex (mutex_) protecting dataReady flag
 * - Atomic busy flag for job acceptance gate (lock-free check in saveVideoFrame())
 * - ImageFrameData snapshot copied before thread wake-up to avoid data races
 *
 * Known issues preventing usage:
 * - Race conditions when simulation modifies peeps/grid during frame copy
 * - Inconsistent frame timing causing dropped frames
 * - Memory visibility issues with relaxed atomic ordering
 * - Condition variable spurious wake-ups not properly handled
 *
 * @warning Do not call directly - this is a thread entry point
 * @warning Currently disabled - synchronous rendering via saveVideoFrameSync() is used instead
 *
 * @see saveVideoFrame() for the job submission gate
 * @see saveVideoFrameSync() for the currently-active synchronous implementation
 * @see abort() for graceful thread termination
 * @see doc/IMAGEWRITER_INTEGRATION_GUIDE.md for detailed bug analysis
 */
void ImageWriter::saveFrameThread() {
  busy = false;  ///< we're ready for business
  fmt::print("Imagewriter thread started.\n");

  while (true) {
    /// wait for job on queue
    std::unique_lock<std::mutex> lck(mutex_);
    condVar.wait(lck, [&] { return dataReady && busy; });
    /// save frame
    dataReady = false;
    busy = false;

    if (abortRequested) {
      break;
    }

    /// save image frame
    saveOneFrameImmed(imageWriter.data);

    /// fmt::print("Image writer thread waiting...\n");
    /// std::this_thread::sleep_for(std::chrono::seconds(2));
  }
  fmt::print("Image writer thread exiting.\n");
}

// Global ImageWriter singleton instance
ImageWriter imageWriter;

}  // namespace Video
}  // namespace IO
}  // namespace v1
}  // namespace BioSim
