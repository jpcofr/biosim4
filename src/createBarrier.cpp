// createBarrier.cpp

#include <cassert>

#include "simulator.h"

namespace BioSim {

// This generates barrier points, which are grid locations with value
// BARRIER. A list of barrier locations is saved in private member
// Grid::barrierLocations and, for some scenarios, Grid::barrierCenters.
// Those members are available read-only with Grid::getBarrierLocations().
// This function assumes an empty grid. This is typically called by
// the main simulator thread after Grid::init() or Grid::zeroFill().

// This file typically is under constant development and change for
// specific scenarios.

void Grid::createBarrier(unsigned barrierType) {
  barrierLocations.clear();
  barrierCenters.clear();  // used only for some barrier types

  auto drawBox = [&](int16_t minX, int16_t minY, int16_t maxX, int16_t maxY) {
    for (int16_t x = minX; x <= maxX; ++x) {
      for (int16_t y = minY; y <= maxY; ++y) {
        grid.set(x, y, BARRIER);
        barrierLocations.push_back({x, y});
      }
    }
  };

  switch (barrierType) {
    case 0:
      return;

    // Vertical bar in constant location
    case 1: {
      int16_t minX = parameterMngrSingleton.gridSize_X / 2;
      int16_t maxX = minX + 1;
      int16_t minY = parameterMngrSingleton.gridSize_Y / 4;
      int16_t maxY = minY + parameterMngrSingleton.gridSize_Y / 2;

      for (int16_t x = minX; x <= maxX; ++x) {
        for (int16_t y = minY; y <= maxY; ++y) {
          grid.set(x, y, BARRIER);
          barrierLocations.push_back({x, y});
        }
      }
    } break;

    // Vertical bar in random location
    case 2: {
      int16_t minX = randomUint(20, parameterMngrSingleton.gridSize_X - 20);
      int16_t maxX = minX + 1;
      int16_t minY = randomUint(20, parameterMngrSingleton.gridSize_Y / 2 - 20);
      int16_t maxY = minY + parameterMngrSingleton.gridSize_Y / 2;

      for (int16_t x = minX; x <= maxX; ++x) {
        for (int16_t y = minY; y <= maxY; ++y) {
          grid.set(x, y, BARRIER);
          barrierLocations.push_back({x, y});
        }
      }
    } break;

    // five blocks staggered
    case 3: {
      int16_t blockSizeX = 2;
      int16_t blockSizeY = parameterMngrSingleton.gridSize_X / 3;

      int16_t x0 = parameterMngrSingleton.gridSize_X / 4 - blockSizeX / 2;
      int16_t y0 = parameterMngrSingleton.gridSize_Y / 4 - blockSizeY / 2;
      int16_t x1 = x0 + blockSizeX;
      int16_t y1 = y0 + blockSizeY;

      drawBox(x0, y0, x1, y1);
      x0 += parameterMngrSingleton.gridSize_X / 2;
      x1 = x0 + blockSizeX;
      drawBox(x0, y0, x1, y1);
      y0 += parameterMngrSingleton.gridSize_Y / 2;
      y1 = y0 + blockSizeY;
      drawBox(x0, y0, x1, y1);
      x0 -= parameterMngrSingleton.gridSize_X / 2;
      x1 = x0 + blockSizeX;
      drawBox(x0, y0, x1, y1);
      x0 = parameterMngrSingleton.gridSize_X / 2 - blockSizeX / 2;
      x1 = x0 + blockSizeX;
      y0 = parameterMngrSingleton.gridSize_Y / 2 - blockSizeY / 2;
      y1 = y0 + blockSizeY;
      drawBox(x0, y0, x1, y1);
      return;
    } break;

    // Horizontal bar in constant location
    case 4: {
      int16_t minX = parameterMngrSingleton.gridSize_X / 4;
      int16_t maxX = minX + parameterMngrSingleton.gridSize_X / 2;
      int16_t minY = parameterMngrSingleton.gridSize_Y / 2 +
                     parameterMngrSingleton.gridSize_Y / 4;
      int16_t maxY = minY + 2;

      for (int16_t x = minX; x <= maxX; ++x) {
        for (int16_t y = minY; y <= maxY; ++y) {
          grid.set(x, y, BARRIER);
          barrierLocations.push_back({x, y});
        }
      }
    } break;

    // Three floating islands -- different locations every generation
    case 5: {
      float radius = 3.0;
      unsigned margin = 2 * (int)radius;

      auto randomLoc = [&]() {
        //                return Coord( (int16_t)randomUint((int)radius +
        //                margin, p.sizeX - ((float)radius + margin)),
        //                              (int16_t)randomUint((int)radius +
        //                              margin, p.sizeY - ((float)radius +
        //                              margin)) );
        return Coordinate(
            (int16_t)randomUint(margin,
                                parameterMngrSingleton.gridSize_X - margin),
            (int16_t)randomUint(margin,
                                parameterMngrSingleton.gridSize_Y - margin));
      };

      Coordinate center0 = randomLoc();
      Coordinate center1;
      Coordinate center2;

      do {
        center1 = randomLoc();
      } while ((center0 - center1).length() < margin);

      do {
        center2 = randomLoc();
      } while ((center0 - center2).length() < margin ||
               (center1 - center2).length() < margin);

      barrierCenters.push_back(center0);
      // barrierCenters.push_back(center1);
      // barrierCenters.push_back(center2);

      auto f = [&](Coordinate loc) {
        grid.set(loc, BARRIER);
        barrierLocations.push_back(loc);
      };

      visitNeighborhood(center0, radius, f);
      // visitNeighborhood(center1, radius, f);
      // visitNeighborhood(center2, radius, f);
    } break;

    // Spots, specified number, radius, locations
    case 6: {
      unsigned numberOfLocations = 5;
      float radius = 5.0;

      auto f = [&](Coordinate loc) {
        grid.set(loc, BARRIER);
        barrierLocations.push_back(loc);
      };

      unsigned verticalSliceSize =
          parameterMngrSingleton.gridSize_Y / (numberOfLocations + 1);

      for (unsigned n = 1; n <= numberOfLocations; ++n) {
        Coordinate loc = {(int16_t)(parameterMngrSingleton.gridSize_X / 2),
                          (int16_t)(n * verticalSliceSize)};
        visitNeighborhood(loc, radius, f);
        barrierCenters.push_back(loc);
      }
    } break;

    default:
      assert(false);
  }
}

}  // namespace BioSim
