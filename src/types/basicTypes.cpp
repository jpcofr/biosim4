/// basicTypes.cpp

#include "basicTypes.h"

#include <cassert>

/**
 * @file basicTypes.cpp
 * @brief Implementation of basic geometric types (Dir, Coordinate, Polar)
 *
 * Provides conversion functions and geometric operations for the fundamental
 * types used throughout the simulator:
 * - Dir: 8 compass directions + center
 * - Coordinate: 2D signed integer position
 * - Polar: Magnitude + direction representation
 *
 * Key operations:
 * - Direction rotation (clockwise/counterclockwise)
 * - Coordinate ↔ direction conversion
 * - Coordinate ↔ polar conversion
 * - Ray similarity calculations
 */

namespace BioSim {
inline namespace v1 {
namespace Types {

/**
 * @brief Compass direction constants for lookup tables
 *
 * Used in rotation and conversion tables to avoid repeated enum casting.
 */
constexpr Compass NW = Compass::NW;
constexpr Compass N = Compass::N;
constexpr Compass NE = Compass::NE;
constexpr Compass E = Compass::E;
constexpr Compass SE = Compass::SE;
constexpr Compass S = Compass::S;
constexpr Compass SW = Compass::SW;
constexpr Compass W = Compass::W;
constexpr Compass C = Compass::CENTER;

/**
 * @brief Rotation lookup table for Dir::rotate()
 *
 * Pre-computed rotation results for all combinations of:
 * - 9 possible directions (0-8)
 * - 8 rotation steps (0-7, representing 0° to 315° in 45° increments)
 *
 * Index calculation: `[sourceDir.asInt() * 8 + (rotationSteps & 7)]`
 *
 * Layout (72 entries = 9 directions × 8 rotation steps):
 * ```
 * Row 0 (SW rotated 0-7 steps): SW, W, NW, N, NE, E, SE, S
 * Row 1 (S  rotated 0-7 steps): S, SW, W, NW, N, NE, E, SE
 * Row 2 (SE rotated 0-7 steps): SE, S, SW, W, NW, N, NE, E
 * ...
 * Row 4 (CENTER): C, C, C, C, C, C, C, C (CENTER never rotates)
 * ...
 * ```
 */
const Dir rotations[72] = {
    Dir(SW), Dir(W),  Dir(NW), Dir(N),  Dir(NE), Dir(E),  Dir(SE), Dir(S),  Dir(S),  Dir(SW), Dir(W), Dir(NW), Dir(N),
    Dir(NE), Dir(E),  Dir(SE), Dir(SE), Dir(S),  Dir(SW), Dir(W),  Dir(NW), Dir(N),  Dir(NE), Dir(E), Dir(W),  Dir(NW),
    Dir(N),  Dir(NE), Dir(E),  Dir(SE), Dir(S),  Dir(SW), Dir(C),  Dir(C),  Dir(C),  Dir(C),  Dir(C), Dir(C),  Dir(C),
    Dir(C),  Dir(E),  Dir(SE), Dir(S),  Dir(SW), Dir(W),  Dir(NW), Dir(N),  Dir(NE),  // NOLINT
    Dir(NW), Dir(N),  Dir(NE), Dir(E),  Dir(SE), Dir(S),  Dir(SW), Dir(W),  Dir(N),  Dir(NE), Dir(E), Dir(SE), Dir(S),
    Dir(SW), Dir(W),  Dir(NW), Dir(NE), Dir(E),  Dir(SE), Dir(S),  Dir(SW), Dir(W),  Dir(NW), Dir(N)};  // NOLINT

/**
 * Implementation of Dir::rotate(int n)
 *
 * Each step represents 45° rotation (8 steps = 360°). Negative values
 * rotate counterclockwise. Uses modulo 8 internally, so rotate(9) = rotate(1).
 *
 * Examples:
 * - `Dir(N).rotate(2)` → `NE` (rotate 90° clockwise)
 * - `Dir(N).rotate(-2)` → `NW` (rotate 90° counterclockwise)
 * - `Dir(N).rotate(4)` → `S` (rotate 180°)
 * - `Dir(CENTER).rotate(n)` → `CENTER` (center never rotates)
 */
Dir Dir::rotate(int n) const {
  return rotations[asInt() * 8 + (n & 7)];
}

/**
 * Lookup table mapping Dir to normalized Coordinate offsets
 *
 * Maps each compass direction to its corresponding unit vector offset.
 * Used for neighbor calculations and movement operations.
 *
 * Mapping (matches Dir::asInt() order):
 * ```
 *  Index  Dir      Coordinate   Meaning
 *  -----  -------  -----------  -------
 *    0    SW       (-1, -1)     Southwest neighbor
 *    1    S        ( 0, -1)     South neighbor
 *    2    SE       ( 1, -1)     Southeast neighbor
 *    3    W        (-1,  0)     West neighbor
 *    4    CENTER   ( 0,  0)     Self (no movement)
 *    5    E        ( 1,  0)     East neighbor
 *    6    NW       (-1,  1)     Northwest neighbor
 *    7    N        ( 0,  1)     North neighbor
 *    8    NE       ( 1,  1)     Northeast neighbor
 * ```
 *
 * @note Y-axis points upward (positive Y = north)
 */
const Coordinate NormalizedCoords[9] = {
    Coordinate(-1, -1),  ///< SW
    Coordinate(0, -1),   ///< S
    Coordinate(1, -1),   ///< SE
    Coordinate(-1, 0),   ///< W
    Coordinate(0, 0),    ///< CENTER
    Coordinate(1, 0),    ///< E
    Coordinate(-1, 1),   ///< NW
    Coordinate(0, 1),    ///< N
    Coordinate(1, 1)     ///< NE
};

/**
 * Implementation of Dir::asNormalizedCoord()
 *
 * Returns the unit vector offset for this direction. Used for
 * calculating neighbor positions and movement deltas.
 *
 * Example:
 * ```cpp
 * Dir(Compass::NE).asNormalizedCoord()  // Returns Coordinate(1, 1)
 * Coordinate playerPos = {10, 20};
 * Coordinate neighborPos = playerPos + Dir(Compass::E).asNormalizedCoord();  // (11, 20)
 * ```
 */
Coordinate Dir::asNormalizedCoord() const {
  return NormalizedCoords[asInt()];
}

/**
 * Implementation of Dir::asNormalizedPolar()
 *
 * Creates a unit polar vector. Useful for directional calculations
 * that require polar representation.
 */
Polar Dir::asNormalizedPolar() const {
  return Polar{1, dir9};
}

/**
 * Implementation of Coordinate::normalize()
 *
 * Converts any coordinate to the closest 8-direction unit vector.
 * First determines the compass direction via asDir(), then converts
 * back to normalized coordinate.
 *
 * Examples:
 * ```cpp
 * Coordinate(5, 3).normalize()    // Returns (1, 1)  - NE direction
 * Coordinate(-10, 0).normalize()  // Returns (-1, 0) - W direction
 * Coordinate(2, -7).normalize()   // Returns (0, -1) - S direction
 * ```
 */
Coordinate Coordinate::normalize() const {
  return asDir().asNormalizedCoord();
}

/**
 * Implementation of Coordinate::asDir()
 *
 * Maps any 2D coordinate to one of the 8 compass directions (or CENTER).
 * Uses an optimized algorithm that rotates the coordinate system by 22.5°
 * to align region boundaries with axes for simpler boundary checks.
 *
 * Algorithm:
 * 1. Rotate system 22.5° clockwise using rational approximation to tan(22.5°)
 * 2. Check which of 16 regions the rotated point falls into (9 are valid)
 * 3. Map region to corresponding compass direction
 *
 * Coordinate space is divided into 45° wedges centered on each direction:
 * ```
 *   NW   N   NE
 *    ╲   │   ╱
 *     ╲  │  ╱
 *   W ─  ●  ─ E
 *     ╱  │  ╲
 *    ╱   │   ╲
 *   SW   S   SE
 * ```
 *
 * @note Uses 32.32 fixed-point arithmetic for precision without floating point
 * @note Algorithm by @Asa-Hopkins
 */
Dir Coordinate::asDir() const {
  /// tanN/tanD is the best rational approximation to tan(22.5) under the
  /// constraint that tanN + tanD < 2**16 (to avoid overflows). We don't care
  /// about the scale of the result, only the ratio of the terms. The actual
  /// rotation is (22.5 - 1.5e-8) degrees, whilst the closest a pair of int16_t's
  /// come to any of these lines is 8e-8 degrees, so the result is exact
  constexpr uint16_t tanN = 13860;
  constexpr uint16_t tanD = 33461;
  const Dir conversion[16]{Dir(S), Dir(C), Dir(SW), Dir(N),  Dir(SE), Dir(E),  Dir(N), Dir(N),
                           Dir(N), Dir(N), Dir(W),  Dir(NW), Dir(N),  Dir(NE), Dir(N), Dir(N)};

  const int32_t xp = x * tanD + y * tanN;
  const int32_t yp = y * tanD - x * tanN;

  /// We can easily check which side of the four boundary lines
  /// the point now falls on, giving 16 cases, though only 9 are
  /// possible.
  return conversion[(yp > 0) * 8 + (xp > 0) * 4 + (yp > xp) * 2 + (yp >= -xp)];
}

/**
 * Implementation of Coordinate::asPolar()
 *
 * Creates polar representation using Euclidean distance as magnitude
 * and compass direction from asDir().
 *
 * Example:
 * ```cpp
 * Coordinate(3, 4).asPolar()  // Returns Polar{5, E} (mag=5, dir=E)
 * ```
 */
Polar Coordinate::asPolar() const {
  return Polar{static_cast<int>(length()), asDir()};
}

/**
 * Implementation of Polar::asCoord()
 *
 * Converts polar (magnitude + direction) back to cartesian (x, y).
 * Uses 32.32 fixed-point arithmetic for precise diagonal calculations.
 *
 * Algorithm:
 * 1. Look up normalized coordinate for direction
 * 2. Scale by magnitude (accounting for √2 factor on diagonals)
 * 3. Round correctly (toward zero for negative, away from zero for positive)
 *
 * Magnitude scaling factors:
 * - Cardinal directions (N, S, E, W): magnitude × 1.0
 * - Diagonal directions (NE, SE, SW, NW): magnitude × 1/√2 ≈ 0.7071
 * - CENTER: always (0, 0) regardless of magnitude
 *
 * @note Uses optimized fixed-point math by @Asa-Hopkins
 */
Coordinate Polar::asCoord() const {
  /// (Thanks to @Asa-Hopkins for this optimized function -- drm)

  /// 3037000500 is 1/sqrt(2) in 32.32 fixed point
  constexpr int64_t coordMags[9] = {
      3037000500,  ///< SW
      1LL << 32,   ///< S
      3037000500,  ///< SE
      1LL << 32,   ///< W
      0,           ///< CENTER
      1LL << 32,   ///< E
      3037000500,  ///< NW
      1LL << 32,   ///< N
      3037000500   ///< NE
  };

  int64_t len = coordMags[dir.asInt()] * mag;

  /// We need correct rounding, the idea here is to add/sub 1/2 (in fixed point)
  /// and truncate. We extend the sign of the magnitude with a cast,
  /// then shift those bits into the lower half, giving 0 for mag >= 0 and
  /// -1 for mag<0. An XOR with this copies the sign onto 1/2, to be exact
  /// we'd then also subtract it, but we don't need to be that precise.

  int64_t temp = (static_cast<int64_t>(mag) >> 32) ^ ((1LL << 31) - 1);
  len = (len + temp) / (1LL << 32);  ///< Divide to make sure we get an arithmetic shift

  const Coordinate basis = NormalizedCoords[dir.asInt()];
  return basis * static_cast<int>(len);
}

/**
 * Implementation of Coordinate::raySameness(Coordinate other)
 *
 * Computes the cosine of the angle between two vectors:
 * - +1.0: Same direction (parallel)
 * -  0.0: Perpendicular directions
 * - -1.0: Opposite directions (antiparallel)
 *
 * Special case: Returns 1.0 if either vector is zero (treats zero as
 * "same as anything").
 *
 * Formula: `cos(θ) = (a·b) / (|a| × |b|)`
 *
 * Example use cases:
 * - Determine if agent is moving toward or away from a target
 * - Calculate alignment bonus/penalty for movement
 * - Check if two agents are facing similar directions
 *
 * @note Uses 64-bit arithmetic to avoid overflow during intermediate calculations
 */
float Coordinate::raySameness(Coordinate other) const {
  int64_t mag = (static_cast<int64_t>(x) * x + y * y) * (other.x * other.x + other.y * other.y);
  if (mag == 0) {
    return 1.0;  ///< anything is "same" as zero vector
  }

  return (x * other.x + y * other.y) / std::sqrt(mag);
}

/**
 * Implementation of Coordinate::raySameness(Dir d)
 *
 * Convenience overload that converts Dir to normalized coordinate before
 * comparing. Returns 1.0 if self is (0,0) or d is CENTER.
 *
 * Example:
 * ```cpp
 * Coordinate velocity(3, 4);
 * float alignmentWithNorth = velocity.raySameness(Dir(Compass::N));
 * ```
 */
float Coordinate::raySameness(Dir d) const {
  return raySameness(d.asNormalizedCoord());
}

}  // namespace Types
}  // namespace v1
}  // namespace BioSim
