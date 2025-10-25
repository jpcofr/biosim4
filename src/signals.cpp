/**
 * @file signals.cpp
 * @brief Implementation of pheromone layer management
 *
 * This file provides the implementation for the Signals system, which manages
 * multiple 2D layers of pheromone signals. Agents can deposit signals at
 * specific locations, and these signals naturally fade over time, enabling
 * indirect communication and emergent behaviors.
 *
 * @see signals.h for the Signals interface documentation
 */

#include "simulator.h"

#include <cstdint>

namespace BioSim {

/**
 * @brief Initialize the signal layers with specified dimensions
 *
 * Allocates memory for all pheromone layers and initializes them to zero.
 * Must be called before any signal operations are performed.
 *
 * @param numLayers Number of independent pheromone layers to create
 * @param sizeX Width of each layer (grid columns)
 * @param sizeY Height of each layer (grid rows)
 *
 * @note Memory is allocated for numLayers × sizeX × sizeY uint8_t values
 * @note All signal values are initialized to 0
 */
void Signals::initialize(uint16_t numLayers, uint16_t sizeX, uint16_t sizeY) {
  data = std::vector<Layer>(numLayers, Layer(sizeX, sizeY));
}

/**
 * @brief Increment signal at location and its neighbors
 *
 * Deposits a pheromone signal at the specified location with diffusion to
 * neighboring cells. The center cell receives a stronger increment than
 * surrounding cells within the diffusion radius.
 *
 * @param layerNum Index of the pheromone layer to modify (0-based)
 * @param loc Grid coordinate where signal is deposited
 *
 * @details
 * - Center cell is incremented by centerIncreaseAmount (2)
 * - Cells within radius 1.5 are incremented by neighborIncreaseAmount (1)
 * - Radius 1.5 includes the 8-connected neighborhood (N/S/E/W + diagonals)
 * - All increments saturate at SIGNAL_MAX (255)
 * - Thread-safe via OpenMP critical section
 *
 * @note Uses global `pheromones` object for storage
 * @note Radius of 1.5 ensures all 8-connected neighbors are affected
 *
 * @see visitNeighborhood() for the neighborhood iteration pattern
 * @see SIGNAL_MAX for saturation limit (255)
 */
void Signals::increment(uint16_t layerNum, Coordinate loc) {
  constexpr float radius = 1.5;
  constexpr uint8_t centerIncreaseAmount = 2;
  constexpr uint8_t neighborIncreaseAmount = 1;

  // Apply increments in OpenMP critical section for thread safety
#pragma omp critical
  {
    // Increment all cells within diffusion radius (8-connected neighborhood)
    visitNeighborhood(loc, radius, [layerNum](Coordinate loc) {
      if (pheromones[layerNum][loc.x][loc.y] < SIGNAL_MAX) {
        pheromones[layerNum][loc.x][loc.y] =
            std::min<unsigned>(SIGNAL_MAX, pheromones[layerNum][loc.x][loc.y] + neighborIncreaseAmount);
      }
    });

    // Apply stronger increment to center cell (deposited on top of neighbor increment)
    if (pheromones[layerNum][loc.x][loc.y] < SIGNAL_MAX) {
      pheromones[layerNum][loc.x][loc.y] =
          std::min<unsigned>(SIGNAL_MAX, pheromones[layerNum][loc.x][loc.y] + centerIncreaseAmount);
    }
  }
}

/**
 * @brief Apply decay to all signals in a layer
 *
 * Reduces signal magnitudes across the entire layer to simulate natural
 * diffusion, evaporation, or degradation of pheromones over time. This
 * creates a natural "forgetting" mechanism for stale signals.
 *
 * @param layerNum Index of the layer to fade (0-based)
 *
 * @details
 * - Each cell is decremented by fadeAmount (1) if above 0
 * - Values are clamped to 0 (no underflow)
 * - Iterates over entire grid (gridSize_X × gridSize_Y)
 *
 * @note Currently has a bug: line `pheromones[layerNum][x][y] = 0;` zeroes
 *       all cells before fading, effectively clearing the layer instead of
 *       decaying it gradually. This should be removed to fix the fade behavior.
 *
 * @warning Bug: The zero assignment before conditional decrement prevents
 *          proper fading behavior
 *
 * @see parameterMngrSingleton.gridSize_X
 * @see parameterMngrSingleton.gridSize_Y
 */
void Signals::fade(unsigned layerNum) {
  constexpr unsigned fadeAmount = 1;

  for (int16_t x = 0; x < parameterMngrSingleton.gridSize_X; ++x)
    for (int16_t y = 0; y < parameterMngrSingleton.gridSize_Y; ++y) {
      pheromones[layerNum][x][y] = 0;  ///< @bug This line clears the cell before fading
      if (pheromones[layerNum][x][y] >= fadeAmount)
        pheromones[layerNum][x][y] -= fadeAmount;  // Decrement signal (dead code due to bug)
    }
}

}  // namespace BioSim
