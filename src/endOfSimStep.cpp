/**
 * @file endOfSimStep.cpp
 * @brief End-of-simulation-step processing and world state maintenance
 *
 * This file contains the logic executed after all individuals have completed
 * their actions for a single simulation step. It handles challenge-specific
 * processing, deferred operations, environment updates, and video frame capture.
 */

#include "imageWriter.h"
#include "simulator.h"

#include <spdlog/fmt/fmt.h>

#include <cmath>
#include <iostream>

namespace BioSim {

/**
 * @brief Process end-of-step operations for the simulation
 *
 * This function is called in single-threaded mode after all individuals have
 * completed their actions for the current simulation step. It performs critical
 * world maintenance and challenge-specific processing in the following order:
 *
 * 1. **Challenge Processing**: Applies challenge-specific rules:
 *    - CHALLENGE_RADIOACTIVE_WALLS: Probabilistically kills agents near radioactive walls
 *    - CHALLENGE_TOUCH_ANY_WALL: Flags individuals touching any arena boundary
 *    - CHALLENGE_LOCATION_SEQUENCE: Tracks sequential barrier visits
 *
 * 2. **Deferred Queue Processing**: Executes queued operations:
 *    - Death queue: Removes individuals marked for elimination
 *    - Movement queue: Applies deferred position changes
 *
 * 3. **Environment Updates**: Modifies world state:
 *    - Fades pheromone signal layers over time
 *
 * 4. **Video Capture**: Optionally saves current world state as video frame
 *    (controlled by videoStride and videoSaveFirstFrames parameters)
 *
 * @param simStep Current simulation step within the generation (0 to stepsPerGeneration-1)
 * @param generation Current generation number (0-based)
 *
 * @note This function must be called in single-threaded context to avoid race
 *       conditions when modifying shared state (grid, peeps, pheromones)
 *
 * @see peeps.drainDeathQueue() for deferred death processing
 * @see peeps.drainMoveQueue() for deferred movement processing
 * @see pheromones.fade() for signal layer decay
 * @see imageWriter.saveVideoFrameSync() for frame capture
 */
void endOfSimulationStep(unsigned simStep, unsigned generation) {
  // ============================================================================
  // CHALLENGE: Radioactive Walls
  // ============================================================================
  /**
   * Radioactive wall challenge implements time-varying environmental hazard:
   * - First half of generation: West wall (X=0) is radioactive
   * - Second half of generation: East wall (X=gridSize_X-1) is radioactive
   *
   * Death probability follows inverse distance relationship:
   *   P(death) = 1 / distance_from_wall
   *
   * Radioactivity has exponential falloff, reaching zero at arena midline.
   * This creates selective pressure for individuals to migrate away from
   * the currently active radioactive wall.
   */
  if (parameterMngrSingleton.challenge == CHALLENGE_RADIOACTIVE_WALLS) {
    // Determine which wall is currently radioactive based on generation progress
    // Determine which wall is currently radioactive based on generation progress
    int16_t radioactiveX =
        (simStep < parameterMngrSingleton.stepsPerGeneration / 2) ? 0 : parameterMngrSingleton.gridSize_X - 1;

    // Iterate through all living individuals and apply radiation exposure
    for (uint16_t index = 1; index <= parameterMngrSingleton.population; ++index) {  // index 0 is reserved
      Individual& indiv = peeps[index];
      if (indiv.alive) {
        // Calculate Manhattan distance from the radioactive wall
        int16_t distanceFromRadioactiveWall = std::abs(indiv.loc.x - radioactiveX);

        // Only apply radiation within half-arena radius (exponential falloff zone)
        if (distanceFromRadioactiveWall < parameterMngrSingleton.gridSize_X / 2) {
          // Death probability = 1/distance (closer = more dangerous)
          float chanceOfDeath = 1.0 / distanceFromRadioactiveWall;

          // Roll dice to determine if individual dies from radiation exposure
          if (randomUint() / (float)RANDOM_UINT_MAX < chanceOfDeath) {
            peeps.queueForDeath(indiv);
          }
        }
      }
    }
  }

  // ============================================================================
  // CHALLENGE: Touch Any Wall
  // ============================================================================
  /**
   * Touch-any-wall challenge rewards individuals that reach arena boundaries.
   * Sets challengeFlag=true for any individual touching a wall (X or Y boundary).
   * At generation end, flagged individuals are selected for reproduction.
   * This creates selective pressure for navigation to arena edges.
   */
  if (parameterMngrSingleton.challenge == CHALLENGE_TOUCH_ANY_WALL) {
    for (uint16_t index = 1; index <= parameterMngrSingleton.population; ++index) {  // index 0 is reserved
      Individual& indiv = peeps[index];

      // Check if individual is touching any of the four arena boundaries
      if (indiv.loc.x == 0 || indiv.loc.x == parameterMngrSingleton.gridSize_X - 1 || indiv.loc.y == 0 ||
          indiv.loc.y == parameterMngrSingleton.gridSize_Y - 1) {
        indiv.challengeBits = true;  // Mark as successful for reproduction
      }
    }
  }

  // ============================================================================
  // CHALLENGE: Location Sequence
  // ============================================================================
  /**
   * Location sequence challenge requires individuals to visit barrier centers
   * in a specific sequential order. Each barrier has a corresponding bit in
   * challengeBits, and bits are set only when barriers are visited in order.
   *
   * Implementation details:
   * - Barrier centers are defined in grid.getBarrierCenters()
   * - Proximity threshold: radius = 9.0 grid units
   * - Bit n is set only if bits 0..(n-1) are already set (enforces ordering)
   * - Loop breaks after checking first unvisited barrier (optimization)
   *
   * This creates selective pressure for path planning and sequential navigation.
   */
  if (parameterMngrSingleton.challenge == CHALLENGE_LOCATION_SEQUENCE) {
    float radius = 9.0;  // Proximity threshold for "visiting" a barrier center

    for (uint16_t index = 1; index <= parameterMngrSingleton.population; ++index) {  // index 0 is reserved
      Individual& indiv = peeps[index];

      // Check each barrier in sequence (order matters!)
      for (unsigned n = 0; n < grid.getBarrierCenters().size(); ++n) {
        unsigned bit = 1 << n;  // Bit mask for barrier n

        // Only check unvisited barriers (bit not yet set)
        if ((indiv.challengeBits & bit) == 0) {
          // Check if individual is within proximity radius of barrier center
          if ((indiv.loc - grid.getBarrierCenters()[n]).length() <= radius) {
            indiv.challengeBits |= bit;  // Set bit to mark this barrier as visited
          }
          // Break after first unvisited barrier (enforces sequential order)
          break;
        }
      }
    }
  }

  // ============================================================================
  // Deferred Operation Processing
  // ============================================================================
  /**
   * Process queued operations that were deferred during parallel individual
   * processing to avoid race conditions:
   *
   * 1. Death Queue: Removes individuals marked for death (from actions,
   *    challenges, or collisions). Updates grid and frees peep indices.
   *
   * 2. Movement Queue: Applies position changes that were validated but
   *    deferred. Updates both individual locations and grid occupancy.
   *
   * These queues enable thread-safe individual processing in the main step loop.
   */
  peeps.drainDeathQueue();
  peeps.drainMoveQueue();

  // ============================================================================
  // Environment Updates
  // ============================================================================
  /**
   * Fade pheromone signals to simulate natural decay over time.
   * Layer 0 is the default pheromone layer (TODO: support multiple layers).
   * Fade rate is controlled by signalSensorRadius parameter.
   */
  pheromones.fade(0);  // Layer number parameter (TODO: make configurable)

  // ============================================================================
  // Deferred Operation Processing
  // ============================================================================
  /**
   * Process queued operations that were deferred during parallel individual
   * processing to avoid race conditions:
   *
   * 1. Death Queue: Removes individuals marked for death (from actions,
   *    challenges, or collisions). Updates grid and frees peep indices.
   *
   * 2. Movement Queue: Applies position changes that were validated but
   *    deferred. Updates both individual locations and grid occupancy.
   *
   * These queues enable thread-safe individual processing in the main step loop.
   */
  peeps.drainDeathQueue();
  peeps.drainMoveQueue();

  // ============================================================================
  // Environment Updates
  // ============================================================================
  /**
   * Fade pheromone signals to simulate natural decay over time.
   * Layer 0 is the default pheromone layer (TODO: support multiple layers).
   * Fade rate is controlled by signalSensorRadius parameter.
   */
  pheromones.fade(0);  ///< takes layerNum  TODO!!!

  // ============================================================================
  // Video Frame Capture
  // ============================================================================
  /**
   * Conditionally save current world state as video frame based on:
   * - saveVideo parameter must be enabled
   * - Generation matches videoStride interval (e.g., every 25th generation)
   * - OR generation is within first videoSaveFirstFrames generations
   * - OR generation is near a parameter change event
   *
   * Uses synchronous frame capture to avoid threading issues.
   * Frames are buffered and converted to video at generation end.
   */
  if (parameterMngrSingleton.saveVideo && ((generation % parameterMngrSingleton.videoStride) == 0 ||
                                           generation <= parameterMngrSingleton.videoSaveFirstFrames ||
                                           (generation >= parameterMngrSingleton.parameterChangeGenerationNumber &&
                                            generation <= parameterMngrSingleton.parameterChangeGenerationNumber +
                                                              parameterMngrSingleton.videoSaveFirstFrames))) {
    // Attempt to save frame synchronously (may fail if imageWriter is busy)
    if (!imageWriter.saveVideoFrameSync(simStep, generation, parameterMngrSingleton.challenge,
                                        parameterMngrSingleton.barrierType)) {
      fmt::print("imageWriter busy\n");  // Non-fatal warning
    }
  }
}

}  // namespace BioSim
