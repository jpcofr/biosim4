#ifndef SIGNALS_H_INCLUDED
#define SIGNALS_H_INCLUDED

// Container for pheromones.

#include <cstdint>
#include <vector>

#include "basicTypes.h"

namespace BioSim {

// Usage: uint8_t magnitude = signals[layer][x][y];
// or             magnitude = signals.getMagnitude(layer, Coord);

constexpr unsigned SIGNAL_MIN = 0;
constexpr unsigned SIGNAL_MAX = UINT8_MAX;

struct Signals {
  struct Column {
    Column(uint16_t numRows) : data{std::vector<uint8_t>(numRows, 0)} {}
    uint8_t& operator[](uint16_t rowNum) { return data[rowNum]; }
    uint8_t operator[](uint16_t rowNum) const { return data[rowNum]; }
    void zeroFill() { std::fill(data.begin(), data.end(), 0); }

   private:
    std::vector<uint8_t> data;
  };

  struct Layer {
    Layer(uint16_t numCols, uint16_t numRows)
        : data{std::vector<Column>(numCols, Column(numRows))} {}
    Column& operator[](uint16_t colNum) { return data[colNum]; }
    const Column& operator[](uint16_t colNum) const { return data[colNum]; }
    void zeroFill() {
      for (Column& col : data) {
        col.zeroFill();
      }
    }

   private:
    std::vector<Column> data;
  };

  void initialize(uint16_t layers, uint16_t sizeX, uint16_t sizeY);
  void increment(uint16_t layerNum, Coordinate loc);
  void fade(unsigned layerNum);

  Layer& operator[](uint16_t layerNum) { return data[layerNum]; }
  const Layer& operator[](uint16_t layerNum) const { return data[layerNum]; }
  uint8_t getMagnitude(uint16_t layerNum, Coordinate loc) const {
    return (*this)[layerNum][loc.x][loc.y];
  }
  void zeroFill() {
    for (Layer& layer : data) layer.zeroFill();
  }

 private:
  std::vector<Layer> data;
};

}  // namespace BioSim

#endif  // SIGNALS_H_INCLUDED
