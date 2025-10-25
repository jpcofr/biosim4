#ifndef BIOSIM4_INCLUDE_GRID_H_
#define BIOSIM4_INCLUDE_GRID_H_

// The grid is the 2D arena where the agents live.

#include "basicTypes.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace BioSim {

/**
 * @class Grid
 * @brief A 2D container of unsigned 16-bit values, with limited understanding
 * of its contents.
 *
 * Grid recognizes elements as EMPTY, BARRIER, or an index value into the
 * 'peeps' container. Elements are allocated and cleared to EMPTY in the
 * constructor.
 *
 * For random element access, prefer using 'at()' and 'set()'. Alternatively,
 * use 'Grid[x][y]' for direct access with 'y' as the inner loop index.
 *
 * Grid does not further interpret the element values.
 */
const uint16_t EMPTY = 0;  // Index value 0 is reserved
const uint16_t BARRIER = 0xffff;

class Grid {
 public:
  /**
   * @struct Column
   * @brief Represents a column in the Grid, allowing access to grid elements as
   * data[x][y].
   *
   * This struct is designed to facilitate access to grid elements using
   * column-major order. It considers 'x' as the column index and 'y' as the row
   * index.
   */
  struct Column {
    Column(uint16_t numRows) : data{std::vector<uint16_t>(numRows, 0)} {}
    void zeroFill() { std::fill(data.begin(), data.end(), 0); }
    uint16_t& operator[](uint16_t row) { return data[row]; }
    uint16_t operator[](uint16_t row) const { return data[row]; }
    size_t size() const { return data.size(); }

   private:
    std::vector<uint16_t> data;
  };

  void initialize(uint16_t sizeX, uint16_t sizeY);
  void zeroFill() {
    for (Column& column : data)
      column.zeroFill();
  }
  uint16_t sizeX() const { return data.size(); }
  uint16_t sizeY() const { return data[0].size(); }
  bool isInBounds(Coordinate location) const {
    return location.x >= 0 && location.x < sizeX() && location.y >= 0 && location.y < sizeY();
  }
  bool isEmptyAt(Coordinate location) const { return at(location) == EMPTY; }
  bool isBarrierAt(Coordinate location) const { return at(location) == BARRIER; }
  // Occupied means an agent is living there.
  bool isOccupiedAt(Coordinate loc) const { return at(loc) != EMPTY && at(loc) != BARRIER; }
  bool isBorder(Coordinate loc) const {
    return loc.x == 0 || loc.x == sizeX() - 1 || loc.y == 0 || loc.y == sizeY() - 1;
  }
  uint16_t at(Coordinate loc) const { return data[loc.x][loc.y]; }
  uint16_t at(uint16_t x, uint16_t y) const { return data[x][y]; }

  void set(Coordinate loc, uint16_t val) { data[loc.x][loc.y] = val; }
  void set(uint16_t x, uint16_t y, uint16_t val) { data[x][y] = val; }
  Coordinate findEmptyLocation() const;
  void createBarrier(unsigned barrierType);
  const std::vector<Coordinate>& getBarrierLocations() const { return barrierLocations; }
  const std::vector<Coordinate>& getBarrierCenters() const { return barrierCenters; }
  // Direct access:
  Column& operator[](uint16_t columnXNum) { return data[columnXNum]; }
  const Column& operator[](uint16_t columnXNum) const { return data[columnXNum]; }

 private:
  std::vector<Column> data;
  std::vector<Coordinate> barrierLocations;
  std::vector<Coordinate> barrierCenters;
};

extern void visitNeighborhood(Coordinate loc, float radius, std::function<void(Coordinate)> f);
extern void unitTestGridVisitNeighborhood();

}  // namespace BioSim

#endif  // BIOSIM4_INCLUDE_GRID_H_
