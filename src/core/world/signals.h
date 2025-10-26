#ifndef BIOSIM4_SRC_CORE_WORLD_SIGNALS_H_
#define BIOSIM4_SRC_CORE_WORLD_SIGNALS_H_

/**
 * @file signals.h
 * @brief Pheromone layer container for agent chemical signaling
 *
 * Provides the Signals struct managing multiple 2D layers of 8-bit pheromone
 * values. Agents can deposit signals and sense them, enabling indirect
 * communication and emergent swarm behaviors.
 *
 * ## Usage
 * ```cpp
 * uint8_t magnitude = signals[layer][x][y];
 * // or
 * magnitude = signals.getMagnitude(layer, coord);
 * ```
 */

#include "../../types/basicTypes.h"

#include <cstdint>
#include <vector>

namespace BioSim {
inline namespace v1 {
namespace Core {
namespace World {

/// Minimum signal magnitude
constexpr unsigned SIGNAL_MIN = 0;

/// Maximum signal magnitude (8-bit)
constexpr unsigned SIGNAL_MAX = UINT8_MAX;

/**
 * @struct Signals
 * @brief Multi-layer 2D container for pheromone signals
 *
 * Manages multiple pheromone layers where agents can deposit and sense
 * chemical signals. Each layer is a 2D grid of 8-bit values (0..255).
 *
 * Signals naturally fade over time and can be used for:
 * - Path marking and trail following
 * - Territorial marking
 * - Danger warnings
 * - Food source indicators
 */
struct Signals {
  /**
   * @struct Column
   * @brief Single column in a signal layer
   */
  struct Column {
    /**
     * @brief Construct column with specified height
     * @param numRows Number of rows
     */
    Column(uint16_t numRows) : data{std::vector<uint8_t>(numRows, 0)} {}

    /**
     * @brief Access cell (non-const)
     * @param rowNum Row index
     * @return Reference to cell value
     */
    uint8_t& operator[](uint16_t rowNum) { return data[rowNum]; }

    /**
     * @brief Access cell (const)
     * @param rowNum Row index
     * @return Cell value
     */
    uint8_t operator[](uint16_t rowNum) const { return data[rowNum]; }

    /**
     * @brief Clear all cells to 0
     */
    void zeroFill() { std::fill(data.begin(), data.end(), 0); }

   private:
    std::vector<uint8_t> data;  ///< Column data storage
  };

  /**
   * @struct Layer
   * @brief Single 2D pheromone layer
   */
  struct Layer {
    /**
     * @brief Construct layer with specified dimensions
     * @param numCols Number of columns (width)
     * @param numRows Number of rows (height)
     */
    Layer(uint16_t numCols, uint16_t numRows) : data{std::vector<Column>(numCols, Column(numRows))} {}

    /**
     * @brief Access column (non-const)
     * @param colNum Column index
     * @return Reference to Column
     */
    Column& operator[](uint16_t colNum) { return data[colNum]; }

    /**
     * @brief Access column (const)
     * @param colNum Column index
     * @return const reference to Column
     */
    const Column& operator[](uint16_t colNum) const { return data[colNum]; }

    /**
     * @brief Clear entire layer to 0
     */
    void zeroFill() {
      for (Column& col : data) {
        col.zeroFill();
      }
    }

   private:
    std::vector<Column> data;  ///< 2D layer data (column-major)
  };

  /**
   * @brief Initialize signal layers with dimensions
   * @param layers Number of pheromone layers
   * @param sizeX Width of each layer
   * @param sizeY Height of each layer
   */
  void initialize(uint16_t layers, uint16_t sizeX, uint16_t sizeY);

  /**
   * @brief Increment signal at location (saturates at SIGNAL_MAX)
   * @param layerNum Layer index
   * @param loc Grid coordinate
   */
  void increment(uint16_t layerNum, Coordinate loc);

  /**
   * @brief Apply decay to all signals in a layer
   * @param layerNum Layer index
   *
   * Reduces signal magnitudes to simulate natural diffusion/evaporation.
   */
  void fade(unsigned layerNum);

  /**
   * @brief Access layer (non-const)
   * @param layerNum Layer index
   * @return Reference to Layer
   */
  Layer& operator[](uint16_t layerNum) { return data[layerNum]; }

  /**
   * @brief Access layer (const)
   * @param layerNum Layer index
   * @return const reference to Layer
   */
  const Layer& operator[](uint16_t layerNum) const { return data[layerNum]; }

  /**
   * @brief Get signal magnitude at specific location
   * @param layerNum Layer index
   * @param loc Grid coordinate
   * @return Signal magnitude (0..255)
   */
  uint8_t getMagnitude(uint16_t layerNum, Coordinate loc) const { return (*this)[layerNum][loc.x][loc.y]; }

  /**
   * @brief Clear all layers to 0
   */
  void zeroFill() {
    for (Layer& layer : data)
      layer.zeroFill();
  }

 private:
  std::vector<Layer> data;  ///< All pheromone layers
};

}  // namespace World
}  // namespace Core
}  // namespace v1

// Backward compatibility aliases
using Core::World::SIGNAL_MAX;
using Core::World::SIGNAL_MIN;
using Core::World::Signals;

}  // namespace BioSim

#endif  // BIOSIM4_SRC_CORE_WORLD_SIGNALS_H_
