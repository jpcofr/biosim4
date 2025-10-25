#ifndef BIOSIM_IMAGEWRITER_H_INCLUDED
#define BIOSIM_IMAGEWRITER_H_INCLUDED

/**
 * @file imageWriter.h
 * @brief Video frame generation and movie assembly for evolution simulation visualization.
 *
 * This module handles the creation of graphic frames for each simulation step and
 * assembles them into video files at the end of each generation. It supports both
 * synchronous and asynchronous (threaded) frame capture modes.
 *
 * @note Async threading exists but is currently disabled due to bugs.
 * See doc/IMAGEWRITER_INTEGRATION_GUIDE.md for details.
 */

#include "indiv.h"
#include "params.h"
#include "peeps.h"

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

namespace BioSim {

/**
 * @struct ImageFrameData
 * @brief Encapsulates all data required to render a single simulation frame.
 *
 * This structure caches snapshot data from the simulation state so that frame
 * rendering can occur asynchronously in a separate thread without blocking the
 * main simulation loop. This allows the simulation to proceed to the next step
 * while the previous frame is being written to disk.
 *
 * @note Currently used in synchronous mode only due to threading bugs.
 */
struct ImageFrameData {
  unsigned simStep;                                       ///< Current step number within the generation
  unsigned generation;                                    ///< Current generation number
  unsigned challenge;                                     ///< Active challenge/selection criterion ID
  unsigned barrierType;                                   ///< Type of barrier in the environment (if any)
  std::vector<Coordinate> indivLocs;                      ///< Locations of all individuals in the grid
  std::vector<uint8_t> indivColors;                       ///< Color values for each individual (visualization)
  std::vector<Coordinate> barrierLocs;                    ///< Locations of barrier cells in the grid
  typedef std::vector<std::vector<uint8_t>> SignalLayer;  ///< 2D signal layer [x][y]
  std::vector<SignalLayer> signalLayers;                  ///< All pheromone/signal layers [layer][x][y]
};

/**
 * @struct ImageWriter
 * @brief Manages video frame capture and movie generation for simulation visualization.
 *
 * The ImageWriter creates visual representations of each simulation step and assembles
 * them into video files at the end of each generation. It buffers frame data and supports
 * both synchronous (blocking) and asynchronous (threaded) capture modes.
 *
 * @par Threading Model
 * The class includes infrastructure for asynchronous frame writing using a dedicated
 * worker thread. However, this functionality is currently disabled due to bugs.
 * Use saveVideoFrameSync() for synchronous, blocking frame capture.
 *
 * @par Usage Pattern
 * 1. Call init() once at startup with grid dimensions
 * 2. Call startNewGeneration() at the beginning of each generation
 * 3. Call saveVideoFrameSync() for each simulation step to capture
 * 4. Call saveGenerationVideo() at generation end to create the movie file
 *
 * @see doc/IMAGEWRITER_INTEGRATION_GUIDE.md for implementation details
 */
struct ImageWriter {
  /**
   * @brief Default constructor.
   */
  ImageWriter();

  /**
   * @brief Initialize the image writer with simulation grid dimensions.
   *
   * Must be called once before any frame capture operations. Sets up internal
   * buffers and prepares the writer for frame capture operations.
   *
   * @param layers Number of signal/pheromone layers to visualize
   * @param sizeX Width of the simulation grid in cells
   * @param sizeY Height of the simulation grid in cells
   */
  void init(uint16_t layers, uint16_t sizeX, uint16_t sizeY);

  /**
   * @brief Reset frame buffers at the start of a new generation.
   *
   * Clears any accumulated frame data from the previous generation and
   * prepares the writer for a new sequence of frames.
   */
  void startNewGeneration();

  /**
   * @brief Asynchronously capture a video frame (CURRENTLY DISABLED).
   *
   * This method queues frame data for asynchronous writing by the worker thread.
   * However, async mode is currently disabled due to threading bugs.
   * Use saveVideoFrameSync() instead.
   *
   * @param simStep Current simulation step number
   * @param generation Current generation number
   * @param challenge Active challenge/selection criterion ID
   * @param barrierType Type of environmental barrier (if any)
   * @return true if frame was successfully queued, false if busy/dropped
   *
   * @warning Do not use - async threading is disabled. Use saveVideoFrameSync().
   * @see saveVideoFrameSync()
   */
  bool saveVideoFrame(unsigned simStep, unsigned generation, unsigned challenge, unsigned barrierType);

  /**
   * @brief Synchronously capture a video frame (RECOMMENDED).
   *
   * Captures the current simulation state as a video frame in blocking mode.
   * This is the preferred method as async threading is currently disabled.
   * Snapshots grid state, individual locations, and signal layers, then
   * renders and buffers the frame for later video assembly.
   *
   * @param simStep Current simulation step number within the generation
   * @param generation Current generation number
   * @param challenge Active challenge/selection criterion ID
   * @param barrierType Type of environmental barrier configuration
   * @return true if frame was successfully captured and buffered
   *
   * @note Respects videoStride and videoSaveFirstFrames config parameters.
   */
  bool saveVideoFrameSync(unsigned simStep, unsigned generation, unsigned challenge, unsigned barrierType);

  /**
   * @brief Assemble buffered frames into a generation video file.
   *
   * Processes all captured frames from the current generation and creates
   * an AVI video file in output/images/. Clears frame buffers after completion.
   *
   * @param generation Generation number (used in output filename)
   *
   * @note Output filename format: output/images/gen-NNNN.avi
   * @note Video codec and format controlled by OpenCV configuration
   */
  void saveGenerationVideo(unsigned generation);

  /**
   * @brief Request immediate termination of async frame writing thread.
   *
   * Signals the worker thread to abort any pending operations and exit cleanly.
   * Only relevant if async mode were enabled.
   *
   * @note Currently has no effect since async threading is disabled.
   */
  void abort();

  /**
   * @brief Worker thread main loop for asynchronous frame writing.
   *
   * This method runs in a separate thread and processes frame data from the
   * queue when available. Currently not used as async mode is disabled.
   *
   * @warning Internal implementation - do not call directly.
   */
  void saveFrameThread();

  /**
   * @brief Count of frames dropped due to thread being busy.
   *
   * Tracks how many frames were skipped because the async writer thread
   * couldn't keep up with the simulation rate. Should remain zero in
   * synchronous mode.
   *
   * @note Atomic to allow safe concurrent access from multiple threads.
   */
  std::atomic<unsigned> droppedFrameCount;

 private:
  std::atomic<bool> busy;           ///< Flag indicating worker thread is processing a frame
  std::mutex mutex_;                ///< Protects shared data access between threads
  std::condition_variable condVar;  ///< Signals worker thread when new data is available
  bool dataReady;                   ///< Flag indicating new frame data is queued

  ImageFrameData data;     ///< Cached frame data for async processing
  bool abortRequested;     ///< Flag to signal worker thread to terminate
  unsigned skippedFrames;  ///< Internal counter for frames skipped during async mode
};

/**
 * @brief Global singleton instance of the ImageWriter.
 *
 * This extern declaration makes the single ImageWriter instance available
 * throughout the codebase. The actual instance is defined and initialized
 * in simulator.cpp.
 *
 * @note Must be initialized via imageWriter.init() before use.
 */
extern ImageWriter imageWriter;

}  // namespace BioSim

#endif  ///< BIOSIM_IMAGEWRITER_H_INCLUDED
