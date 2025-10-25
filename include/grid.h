#ifndef BIOSIM4_INCLUDE_GRID_H_
#define BIOSIM4_INCLUDE_GRID_H_

/**
 * @file grid.h
 * @brief 2D simulation arena where agents live and move
 *
 * Provides the Grid class which manages a 2D array of 16-bit values representing:
 * - EMPTY (0): unoccupied cell
 * - BARRIER (0xffff): impassable obstacle
 * - Index (1..0xfffe): reference to Individual in peeps[] container
 */

#include "basicTypes.h"

#include <cstdint>
#include <functional>
#include <vector>

namespace BioSim {

/// Special grid value indicating empty cell (index 0 is reserved)
const uint16_t EMPTY = 0;

/// Special grid value indicating barrier/obstacle
const uint16_t BARRIER = 0xffff;

/**
 * @class Grid
 * @brief 2D container of unsigned 16-bit values representing the simulation arena
 *
 * Grid manages a 2D array where each cell can be:
 * - EMPTY (no agent present)
 * - BARRIER (impassable obstacle)
 * - An index into the peeps[] container (agent location)
 *
 * ## Access Patterns
 * - **Random access**: Use at() and set() methods
 * - **Direct access**: Use Grid[x][y] with y as inner loop index (column-major)
 *
 * Grid does not interpret element values beyond the three categories above.
 */
class Grid {
 public:
  /**
   * @struct Column
   * @brief Represents one column in the Grid for data[x][y] access
   *
   * Facilitates column-major order access where x is the column index
   * and y is the row index.
   */
  struct Column {
    /**
     * @brief Construct a column with specified height
     * @param numRows Number of rows in column
     */
    Column(uint16_t numRows) : data{std::vector<uint16_t>(numRows, 0)} {}

    /**
     * @brief Clear all cells in column to 0
     */
    void zeroFill() { std::fill(data.begin(), data.end(), 0); }

    /**
     * @brief Access cell in column (non-const)
     * @param row Row index
     * @return Reference to cell value
     */
    uint16_t& operator[](uint16_t row) { return data[row]; }

    /**
     * @brief Access cell in column (const)
     * @param row Row index
     * @return Cell value
     */
    uint16_t operator[](uint16_t row) const { return data[row]; }

    /**
     * @brief Get column height
     * @return Number of rows
     */
    size_t size() const { return data.size(); }

   private:
    std::vector<uint16_t> data;  ///< Column data storage
  };

  /**
   * @brief Initialize grid with specified dimensions
   * @param sizeX Width (columns)
   * @param sizeY Height (rows)
   */
  void initialize(uint16_t sizeX, uint16_t sizeY);

  /**
   * @brief Clear entire grid to 0
   */
  void zeroFill() {
    for (Column& column : data)
      column.zeroFill();
  }

  /**
   * @brief Get grid width
   * @return Number of columns
   */
  uint16_t sizeX() const { return data.size(); }

  /**
   * @brief Get grid height
   * @return Number of rows
   */
  uint16_t sizeY() const { return data[0].size(); }

  /**
   * @brief Check if coordinate is within grid bounds
   * @param location Coordinate to check
   * @return true if location is valid
   */
  bool isInBounds(Coordinate location) const {
    return location.x >= 0 && location.x < sizeX() && location.y >= 0 && location.y < sizeY();
  }

  /**
   * @brief Check if cell is empty (no agent, no barrier)
   * @param location Cell coordinate
   * @return true if cell equals EMPTY
   */
  bool isEmptyAt(Coordinate location) const { return at(location) == EMPTY; }

  /**
   * @brief Check if cell contains a barrier
   * @param location Cell coordinate
   * @return true if cell equals BARRIER
   */
  bool isBarrierAt(Coordinate location) const { return at(location) == BARRIER; }

  /**
   * @brief Check if cell is occupied by an agent
   * @param loc Cell coordinate
   * @return true if cell contains agent index (not EMPTY or BARRIER)
   */
  bool isOccupiedAt(Coordinate loc) const { return at(loc) != EMPTY && at(loc) != BARRIER; }

  /**
   * @brief Check if coordinate is on grid border
   * @param loc Coordinate to check
   * @return true if on any edge
   */
  bool isBorder(Coordinate loc) const {
    return loc.x == 0 || loc.x == sizeX() - 1 || loc.y == 0 || loc.y == sizeY() - 1;
  }

  /**
   * @brief Get value at coordinate
   * @param loc Grid coordinate
   * @return Cell value (EMPTY, BARRIER, or agent index)
   */
  uint16_t at(Coordinate loc) const { return data[loc.x][loc.y]; }

  /**
   * @brief Get value at x,y position
   * @param x Column index
   * @param y Row index
   * @return Cell value
   */
  uint16_t at(uint16_t x, uint16_t y) const { return data[x][y]; }

  /**
   * @brief Set value at coordinate
   * @param loc Grid coordinate
   * @param val Value to set
   */
  void set(Coordinate loc, uint16_t val) { data[loc.x][loc.y] = val; }

  /**
   * @brief Set value at x,y position
   * @param x Column index
   * @param y Row index
   * @param val Value to set
   */
  void set(uint16_t x, uint16_t y, uint16_t val) { data[x][y] = val; }

  /**
   * @brief Find a random empty cell
   * @return Coordinate of empty location
   */
  Coordinate findEmptyLocation() const;

  /**
   * @brief Create barriers in grid according to type
   * @param barrierType Barrier configuration ID
   */
  void createBarrier(unsigned barrierType);

  /**
   * @brief Get all barrier locations
   * @return Vector of coordinates containing barriers
   */
  const std::vector<Coordinate>& getBarrierLocations() const { return barrierLocations; }

  /**
   * @brief Get barrier cluster centers
   * @return Vector of coordinates at center of barrier groups
   */
  const std::vector<Coordinate>& getBarrierCenters() const { return barrierCenters; }

  /**
   * @brief Direct column access (non-const)
   * @param columnXNum Column index
   * @return Reference to Column
   */
  Column& operator[](uint16_t columnXNum) { return data[columnXNum]; }

  /**
   * @brief Direct column access (const)
   * @param columnXNum Column index
   * @return const reference to Column
   */
  const Column& operator[](uint16_t columnXNum) const { return data[columnXNum]; }

 private:
  std::vector<Column> data;                  ///< 2D grid data (column-major)
  std::vector<Coordinate> barrierLocations;  ///< All barrier cell coordinates
  std::vector<Coordinate> barrierCenters;    ///< Centers of barrier clusters
};

/**
 * @brief Visit all cells within circular radius of a location
 * @param loc Center coordinate
 * @param radius Search radius (grid units)
 * @param f Function to call for each cell within radius
 *
 * Executes lambda over circular neighborhood. Only visits cells within grid bounds.
 * - Radius 1.0 = center + 4 neighbors (N/S/E/W)
 * - Radius 1.5 = center + 8 neighbors (includes diagonals)
 */
extern void visitNeighborhood(Coordinate loc, float radius, std::function<void(Coordinate)> f);

/**
 * @brief Unit test for visitNeighborhood function
 */
extern void unitTestGridVisitNeighborhood();

}  // namespace BioSim

#endif  ///< BIOSIM4_INCLUDE_GRID_H_
