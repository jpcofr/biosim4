#ifndef BIOSIM4_INCLUDE_BASICTYPES_H_
#define BIOSIM4_INCLUDE_BASICTYPES_H_

/**
 * @file basicTypes.h
 * @brief Basic geometric types used throughout the project
 *
 * This file defines core types for direction and position handling:
 *
 * - **Compass**: Enum with 9 directions (SW, S, SE, W, CENTER, E, NW, N, NE)
 *   - Arithmetic values arranged as:
 *     ```
 *     6  7  8
 *     3  4  5
 *     0  1  2
 *     ```
 *
 * - **Dir**: Abstract type for 8 directions plus center
 *   - Constructor: `Dir(Compass = CENTER)`
 *
 * - **Coordinate**: Signed int16_t pair for absolute/relative locations
 *   - Constructor: `Coordinate(x=0, y=0)`
 *
 * - **Polar**: Signed magnitude and direction
 *   - Constructor: `Polar(mag=0, dir=CENTER)`
 *
 * ## Conversions
 * - `uint8_t = Dir.asInt()`
 * - `Dir = Coordinate.asDir()` or `Polar.asDir()`
 * - `Coordinate = Dir.asNormalizedCoord()` or `Polar.asCoord()`
 * - `Polar = Dir.asNormalizedPolar()` or `Coordinate.asPolar()`
 *
 * ## Arithmetic Operations
 * - `Dir.rotate(int n)`
 * - `Coordinate = Coordinate + Dir/Coordinate/Polar`
 * - `Polar = Polar + Coordinate/Polar` (additive)
 * - `Polar = Polar * Polar` (dot product)
 */

#include "random.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace BioSim {

/**
 * @brief Unit test function for basic types
 * @return true if all tests pass, false otherwise
 */
extern bool unitTestBasicTypes();

/**
 * @enum Compass
 * @brief Enumerates 9 compass directions including center
 *
 * Arithmetic values:
 * ```
 * 6  7  8
 * 3  4  5
 * 0  1  2
 * ```
 */
enum class Compass : uint8_t { SW = 0, S, SE, W, CENTER, E, NW, N, NE };

struct Dir;
struct Coordinate;
struct Polar;

/**
 * @struct Dir
 * @brief Represents a direction in 8 compass directions plus center
 *
 * Supports the eight directions in enum class Compass plus CENTER.
 * Packed to minimize memory footprint.
 */
struct __attribute__((packed)) Dir {
  /**
   * @brief Generate a random direction (excludes CENTER)
   * @return Random Dir from 8 compass directions
   */
  static Dir random8() { return Dir(Compass::N).rotate(randomUint(0, 7)); }

  /**
   * @brief Construct a direction
   * @param dir Initial compass direction (default: CENTER)
   */
  Dir(Compass dir = Compass::CENTER) : dir9{dir} {}

  /**
   * @brief Assign from Compass enum
   * @param d Compass direction to assign
   * @return Reference to this Dir
   */
  Dir& operator=(const Compass& d) {
    dir9 = d;
    return *this;
  }

  /**
   * @brief Convert direction to integer representation
   * @return uint8_t value 0-8
   */
  uint8_t asInt() const { return (uint8_t)dir9; }

  /**
   * @brief Convert to normalized coordinate (-1, 0, or 1 for x and y)
   * @return Coordinate with components in range [-1, 1]
   */
  Coordinate asNormalizedCoord() const;

  /**
   * @brief Convert to normalized polar representation
   * @return Polar with magnitude 0 or 1
   */
  Polar asNormalizedPolar() const;

  /**
   * @brief Rotate direction by n compass steps
   * @param n Number of 45-degree steps (positive = clockwise)
   * @return Rotated direction
   */
  Dir rotate(int n = 0) const;

  /**
   * @brief Rotate 90 degrees clockwise
   * @return Rotated direction
   */
  Dir rotate90DegCW() const { return rotate(2); }

  /**
   * @brief Rotate 90 degrees counter-clockwise
   * @return Rotated direction
   */
  Dir rotate90DegCCW() const { return rotate(-2); }

  /**
   * @brief Rotate 180 degrees
   * @return Opposite direction
   */
  Dir rotate180Deg() const { return rotate(4); }

  bool operator==(Compass d) const { return asInt() == (uint8_t)d; }
  bool operator!=(Compass d) const { return asInt() != (uint8_t)d; }
  bool operator==(Dir d) const { return asInt() == d.asInt(); }
  bool operator!=(Dir d) const { return asInt() != d.asInt(); }

 private:
  Compass dir9;  ///< Internal compass direction storage
};

/**
 * @struct Coordinate
 * @brief 2D coordinate with signed 16-bit components
 *
 * Coordinates can represent absolute positions in the simulator grid
 * or relative offsets between positions. Arithmetic wraps like int16_t.
 */
struct __attribute__((packed)) Coordinate {
  /**
   * @brief Construct a coordinate
   * @param x0 X component (default: 0)
   * @param y0 Y component (default: 0)
   */
  Coordinate(int16_t x0 = 0, int16_t y0 = 0) : x{x0}, y{y0} {}

  /**
   * @brief Check if coordinate is normalized (components in [-1, 1])
   * @return true if both x and y are in range [-1, 1]
   */
  bool isNormalized() const { return x >= -1 && x <= 1 && y >= -1 && y <= 1; }

  /**
   * @brief Normalize coordinate to unit length
   * @return Coordinate with components in [-1, 1]
   */
  Coordinate normalize() const;

  /**
   * @brief Calculate Euclidean length (rounded down)
   * @return unsigned distance from origin
   */
  unsigned length() const { return (int)(std::sqrt(x * x + y * y)); }

  /**
   * @brief Convert coordinate to nearest compass direction
   * @return Dir representing the direction
   */
  Dir asDir() const;

  /**
   * @brief Convert to polar representation
   * @return Polar with magnitude and direction
   */
  Polar asPolar() const;

  bool operator==(Coordinate c) const { return x == c.x && y == c.y; }
  bool operator!=(Coordinate c) const { return x != c.x || y != c.y; }
  Coordinate operator+(Coordinate c) const { return Coordinate{(int16_t)(x + c.x), (int16_t)(y + c.y)}; }
  Coordinate operator-(Coordinate c) const { return Coordinate{(int16_t)(x - c.x), (int16_t)(y - c.y)}; }
  Coordinate operator*(int a) const { return Coordinate{(int16_t)(x * a), (int16_t)(y * a)}; }
  Coordinate operator+(Dir d) const { return *this + d.asNormalizedCoord(); }
  Coordinate operator-(Dir d) const { return *this - d.asNormalizedCoord(); }

  /**
   * @brief Calculate directional similarity with another coordinate
   * @param other Coordinate to compare with
   * @return float in range [-1.0, 1.0] where 1.0 is same direction, -1.0 is opposite
   */
  float raySameness(Coordinate other) const;

  /**
   * @brief Calculate directional similarity with a direction
   * @param d Direction to compare with
   * @return float in range [-1.0, 1.0] where 1.0 is same direction, -1.0 is opposite
   */
  float raySameness(Dir d) const;

 public:
  int16_t x;  ///< X component
  int16_t y;  ///< Y component
};

/**
 * @struct Polar
 * @brief Polar coordinate with signed 32-bit magnitude and direction
 *
 * Magnitudes are signed 32-bit integers to extend across any 2D area
 * defined by the Coordinate class.
 */
struct __attribute__((packed)) Polar {
  /**
   * @brief Construct polar coordinate
   * @param mag0 Magnitude (default: 0)
   * @param dir0 Direction as Compass (default: CENTER)
   */
  explicit Polar(int mag0 = 0, Compass dir0 = Compass::CENTER) : mag{mag0}, dir{Dir{dir0}} {}

  /**
   * @brief Construct polar coordinate
   * @param mag0 Magnitude
   * @param dir0 Direction as Dir
   */
  explicit Polar(int mag0, Dir dir0) : mag{mag0}, dir{dir0} {}

  /**
   * @brief Convert polar to Cartesian coordinates
   * @return Coordinate representation
   */
  Coordinate asCoord() const;

 public:
  int mag;  ///< Magnitude (signed for bidirectional representation)
  Dir dir;  ///< Direction
};

}  // namespace BioSim

#endif  // BIOSIM4_INCLUDE_BASICTYPES_H_
