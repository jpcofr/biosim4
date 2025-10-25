// grid.cpp

#include "simulator.h"

#include <cassert>
#include <functional>

namespace BioSim {

// Allocates space for the 2D grid
void Grid::initialize(uint16_t size_X, uint16_t size_Y) {
  auto col = Column(size_Y);
  data = std::vector<Column>(size_X, col);
}

// Finds a random unoccupied location in the grid
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
 * This function visits each valid (in-bounds) location in the specified
 * neighborhood around the specified location. It feeds each valid location to
 * the specified function. Locations include self (center of the neighborhood).
 *
 * @param loc The center location of the neighborhood.
 * @param radius The radius of the neighborhood.
 * @param f The function to apply on each valid location in the neighborhood.
 */
void visitNeighborhood(Coordinate loc, float radius, std::function<void(Coordinate)> function) {
  for (int dx = -std::min<int>(radius, loc.x);
       dx <= std::min<int>(radius, (parameterMngrSingleton.gridSize_X - loc.x) - 1); ++dx) {
    int16_t x = loc.x + dx;
    assert(x >= 0 && x < parameterMngrSingleton.gridSize_X);
    int extentY = (int)sqrt(radius * radius - dx * dx);
    for (int dy = -std::min<int>(extentY, loc.y);
         dy <= std::min<int>(extentY, (parameterMngrSingleton.gridSize_Y - loc.y) - 1); ++dy) {
      int16_t y = loc.y + dy;
      assert(y >= 0 && y < parameterMngrSingleton.gridSize_Y);
      function(Coordinate{x, y});
    }
  }
}

}  // namespace BioSim
