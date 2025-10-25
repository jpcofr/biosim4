/// unitTestGridVisitNeighborhood

#include "simulator.h"

#include <spdlog/fmt/fmt.h>

#include <iostream>

namespace BioSim {

void unitTestGridVisitNeighborhood() {
  /// prints each coord:
  auto printLoc = [&](Coordinate loc) { fmt::print("{}, {}\n", loc.x, loc.y); };

  fmt::print("Test loc 10,10 radius 1\n");
  visitNeighborhood(Coordinate{10, 10}, 1.0, printLoc);

  fmt::print("\nTest loc 0,0 radius 1\n");
  visitNeighborhood(Coordinate{0, 0}, 1.0, printLoc);

  fmt::print("\nTest loc 10,10 radius 1.4\n");
  visitNeighborhood(Coordinate{10, 10}, 1.4, printLoc);

  fmt::print("\nTest loc 10,10 radius 1.5\n");
  visitNeighborhood(Coordinate{10, 10}, 1.5, printLoc);

  fmt::print("\nTest loc 1,1 radius 1.4\n");
  visitNeighborhood(Coordinate{1, 1}, 1.4, printLoc);

  fmt::print("\nTest loc 10,10 radius 2.0\n");
  visitNeighborhood(Coordinate{10, 10}, 2.0, printLoc);

  fmt::print("\nTest loc p.sizeX-1, p.sizeY-1 radius 2.0\n");
  visitNeighborhood(
      Coordinate{(int16_t)(parameterMngrSingleton.gridSize_X - 1), (int16_t)(parameterMngrSingleton.gridSize_Y - 1)},
      2.0, printLoc);
}

}  // namespace BioSim
