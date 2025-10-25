/**
 * @file endOfGeneration.cpp
 * @brief Handles end-of-generation tasks for the simulation.
 *
 * This file contains the implementation of the endOfGeneration function, which
 * is called once per generation to perform cleanup and output tasks.
 */

#include "imageWriter.h"
#include "simulator.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <utility>

namespace BioSim {

/**
 * @brief Executes end-of-generation tasks such as saving videos and updating logs.
 *
 * This function is called at the conclusion of each generation to perform cleanup
 * and output operations. It coordinates two main tasks:
 *
 * **Video Generation:**
 * - Saves a video of the completed generation if video saving is enabled
 * - Applies striding logic to control which generations are saved
 * - Saves videos during parameter change periods for comparison
 *
 * **Log Updates:**
 * - Updates graphical statistics logs via external command execution
 * - Applies update stride to control log refresh frequency
 *
 * @param generation The generation number that has just completed (0-based).
 *
 * @details
 * **Video Save Conditions** (all must be true):
 * - `parameterMngrSingleton.saveVideo` is true
 * - At least one of:
 *   - `generation % parameterMngrSingleton.videoStride == 0` (periodic save)
 *   - `generation <= parameterMngrSingleton.videoSaveFirstFrames` (early generations)
 *   - Generation is within parameter change window:
 *     `[parameterChangeGenerationNumber, parameterChangeGenerationNumber + videoSaveFirstFrames]`
 *
 * **Log Update Conditions** (all must be true):
 * - `parameterMngrSingleton.updateGraphLog` is true
 * - Either:
 *   - `generation == 1` (first generation)
 *   - `generation % parameterMngrSingleton.updateGraphLogStride == 0` (periodic update)
 *
 * @see ImageWriter::saveGenerationVideo()
 * @see parameterMngrSingleton Configuration singleton containing all parameters
 *
 * @note This function uses `std::system()` for external command execution, which
 *       suppresses unused-result warnings via GCC pragma.
 *
 * @warning The external command execution is synchronous and will block until complete.
 */
void endOfGeneration(unsigned generation) {
  // Video generation block
  {
    /// Check if this generation should save a video based on stride and special conditions
    if (parameterMngrSingleton.saveVideo && ((generation % parameterMngrSingleton.videoStride) == 0 ||
                                             generation <= parameterMngrSingleton.videoSaveFirstFrames ||
                                             (generation >= parameterMngrSingleton.parameterChangeGenerationNumber &&
                                              generation <= parameterMngrSingleton.parameterChangeGenerationNumber +
                                                                parameterMngrSingleton.videoSaveFirstFrames))) {
      /// Save video frames accumulated during this generation to disk
      imageWriter.saveGenerationVideo(generation);
    }
  }

  // Log update block
  {
    /// Check if graphical logs should be updated this generation
    if (parameterMngrSingleton.updateGraphLog &&
        (generation == 1 || ((generation % parameterMngrSingleton.updateGraphLogStride) == 0))) {
#pragma GCC diagnostic ignored "-Wunused-result"
      /// Execute external command to refresh graphical statistics (e.g., gnuplot)
      std::system(parameterMngrSingleton.graphLogUpdateCommand.c_str());
    }
  }
}

}  // namespace BioSim
