/**
 * @file grid.cpp
 * @brief Implementation of the 2D simulation grid and neighborhood traversal functions
 *
 * This file provides the implementation for:
 * - Grid initialization and memory allocation
 * - Finding empty locations for agent placement
 * - Circular neighborhood traversal for sensor queries and action execution
 */

#include "grid.h"

#include "../../utils/random.h"       // For randomUint
#include "../simulation/simulator.h"  // For grid, parameterMngrSingleton

#include <cassert>
#include <functional>

namespace BioSim {
inline namespace v1 {
namespace Core {
namespace World {

/**
 * @brief Allocates space for the 2D grid
 *
 * Initializes the grid data structure with the specified dimensions. Creates a
 * column-major 2D array where data[x][y] is accessed with x as the column index.
 * All cells are initialized to 0 (EMPTY).
 *
 * @param size_X Width of the grid (number of columns)
 * @param size_Y Height of the grid (number of rows)
 *
 * @note This must be called before any grid operations
 * @note Memory is allocated as std::vector for automatic management
 *
 * @see Grid::Column for the underlying column structure
 */
void Grid::initialize(uint16_t size_X, uint16_t size_Y) {
  auto col = Column(size_Y);
  data = std::vector<Column>(size_X, col);
}

/**
 * @brief Finds a random unoccupied location in the grid
 *
 * Uses a simple rejection sampling approach: repeatedly generates random
 * coordinates until an empty cell is found. This method is efficient when
 * the grid has many empty cells but can become slow as the grid fills up.
 *
 * @return Coordinate A random location where grid.isEmptyAt() returns true
 *
 * @warning This function will loop indefinitely if the grid is completely full
 * @note Uses global parameterMngrSingleton for grid dimensions
 * @note Thread-safe as long as randomUint() is thread-safe
 *
 * @see Grid::isEmptyAt() for the emptiness check
 * @see randomUint() for the random number generation
 */
Coordinate Grid::findEmptyLocation() const {
  Coordinate loc;

  while (true) {
    loc.x = randomUint(0, parameterMngrSingleton.gridSize_X - 1);
    loc.y = randomUint(0, parameterMngrSingleton.gridSize_Y - 1);
    if (grid.isEmptyAt(loc)) {
      break;
    }
  }
  return loc;
}

/**
 * @brief Visits each valid (in-bounds) location in a circular neighborhood
 *
 * This function executes a callback for every grid location within a specified
 * circular radius of a center point. The algorithm uses a circle equation to
 * determine which cells fall within the radius, automatically clipping to grid
 * boundaries. The center location itself is included in the traversal.
 *
 * ## Algorithm
 * - Outer loop iterates over x-coordinates within [-radius, +radius] from center
 * - For each x, calculates maximum y extent: sqrt(radius² - dx²)
 * - Inner loop iterates over valid y-coordinates within this extent
 * - Boundary checking ensures all coordinates stay within grid bounds
 *
 * ## Radius Examples
 * - radius = 1.0: center + 4 neighbors (N, S, E, W) = 5 cells
 * - radius = 1.5: center + 8 neighbors (includes diagonals) = 9 cells
 * - radius = 2.0: center + ~12 neighbors (circular pattern) ≈ 13 cells
 *
 * @param loc The center location of the neighborhood (included in traversal)
 * @param radius The radius of the circular neighborhood in grid units
 * @param function Callback function invoked for each valid location
 *
 * @note The function is called exactly once per location (no duplicates)
 * @note Locations outside grid bounds are automatically skipped
 * @note Uses parameterMngrSingleton for grid dimensions
 * @note All visited coordinates are guaranteed to be valid (asserted)
 *
 * @warning Not thread-safe if the callback modifies shared state
 *
 * @see Grid::isOccupiedAt() common use case for checking occupancy
 * @see Individual::getSensor() uses this for POPULATION and POPULATION_FWD sensors
 * @see Individual::executeActions() uses this for EMIT_SIGNAL action
 *
 * Example usage:
 * @code
 * visitNeighborhood(creature.loc, 1.5, [](Coordinate neighbor) {
 *     if (grid.isOccupiedAt(neighbor)) {
 *         count++;
 *     }
 * });
 * @endcode
 */
void visitNeighborhood(Coordinate loc, float radius, std::function<void(Coordinate)> f) {
  // Iterate over x-coordinates within radius, clipped to grid bounds
  for (int dx = -std::min<int>(radius, loc.x);
       dx <= std::min<int>(radius, (parameterMngrSingleton.gridSize_X - loc.x) - 1); ++dx) {
    int16_t x = loc.x + dx;
    assert(x >= 0 && x < parameterMngrSingleton.gridSize_X);

    // Calculate maximum y extent at this x using circle equation: y = sqrt(r² - x²)
    int extentY = (int)sqrt(radius * radius - dx * dx);

    // Iterate over y-coordinates within circular extent, clipped to grid bounds
    for (int dy = -std::min<int>(extentY, loc.y);
         dy <= std::min<int>(extentY, (parameterMngrSingleton.gridSize_Y - loc.y) - 1); ++dy) {
      int16_t y = loc.y + dy;
      assert(y >= 0 && y < parameterMngrSingleton.gridSize_Y);

      // Invoke callback with this valid in-bounds coordinate
      f(Coordinate{x, y});
    }
  }
}

}  // namespace World
}  // namespace Core
}  // namespace v1
}  // namespace BioSim
