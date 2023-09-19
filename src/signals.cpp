// signals.cpp
// Manages layers of pheremones

#include <cstdint>

#include "simulator.h"

namespace BioSim {

void Signals::initialize(uint16_t numLayers, uint16_t sizeX, uint16_t sizeY) {
  data = std::vector<Layer>(numLayers, Layer(sizeX, sizeY));
}

// Increases the specified location by centerIncreaseAmount,
// and increases the neighboring cells by neighborIncreaseAmount

void Signals::increment(uint16_t layerNum, Coordinate loc) {
  constexpr float radius = 1.5;
  constexpr uint8_t centerIncreaseAmount = 2;
  constexpr uint8_t neighborIncreaseAmount = 1;

#pragma omp critical
  {
    visitNeighborhood(loc, radius, [layerNum](Coordinate loc) {
      if (pheromones[layerNum][loc.x][loc.y] < SIGNAL_MAX) {
        pheromones[layerNum][loc.x][loc.y] =
            std::min<unsigned>(SIGNAL_MAX, pheromones[layerNum][loc.x][loc.y] +
                                               neighborIncreaseAmount);
      }
    });

    if (pheromones[layerNum][loc.x][loc.y] < SIGNAL_MAX) {
      pheromones[layerNum][loc.x][loc.y] =
          std::min<unsigned>(SIGNAL_MAX, pheromones[layerNum][loc.x][loc.y] +
                                             centerIncreaseAmount);
    }
  }
}

// Fades the signals
void Signals::fade(unsigned layerNum) {
  constexpr unsigned fadeAmount = 1;

  for (int16_t x = 0; x < parameterMngrSingleton.gridSize_X; ++x)
    for (int16_t y = 0; y < parameterMngrSingleton.gridSize_Y; ++y) {
      pheromones[layerNum][x][y] = 0;
      if (pheromones[layerNum][x][y] >= fadeAmount)
        pheromones[layerNum][x][y] -= fadeAmount;  // fade center cell
    }
}

}  // namespace BioSim
