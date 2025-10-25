/**
 * @file survival-criteria.cpp
 * @brief Implements survival challenge evaluation logic for evolutionary selection.
 *
 * This file contains the logic for determining which individuals survive each generation
 * based on various challenge types. Each challenge defines different spatial, social, or
 * behavioral criteria that individuals must meet to become parents of the next generation.
 *
 * Challenges range from simple location-based tests (e.g., reach a corner) to complex
 * behavioral requirements (e.g., maintain specific neighbor counts, visit locations in
 * sequence). The scoring system allows both binary pass/fail and weighted selection based
 * on performance quality.
 */

#include "simulator.h"

#include <cassert>
#include <utility>

namespace BioSim {

/**
 * @brief Evaluates whether an individual passed the survival criterion for the given challenge.
 *
 * This function implements the core selection mechanism for evolutionary fitness. Each challenge
 * type defines different survival criteria that individuals must meet at the end of a generation
 * to become parents. The function returns both a pass/fail boolean and a fitness score that can
 * be used for weighted parent selection.
 *
 * @param indiv The individual to evaluate (must contain location, alive status, and challenge bits)
 * @param challenge The challenge type ID (see challenge constants in simulator.h)
 *
 * @return A pair containing:
 *         - bool: true if the individual passed the challenge, false otherwise
 *         - float: fitness score in range [0.0, 1.0] where higher values indicate better performance.
 *                  For binary challenges, score is 1.0 (pass) or 0.0 (fail).
 *                  For weighted challenges, score reflects quality of achievement (e.g., distance to target).
 *
 * @note Dead individuals (indiv.alive == false) automatically fail all challenges with score 0.0
 * @note Some challenges (e.g., CHALLENGE_RADIOACTIVE_WALLS, CHALLENGE_TOUCH_ANY_WALL) involve
 *       partial evaluation in endOfSimStep() that sets flags in indiv.challengeBits
 */
std::pair<bool, float> passedSurvivalCriterion(const Individual& indiv, unsigned challenge) {
  if (!indiv.alive) {
    return {false, 0.0};
  }

  switch (challenge) {
    /**
     * @brief CHALLENGE_CIRCLE - Survive by being inside a circular safe zone.
     *
     * Survivors must be located within a circular area in the lower-left quadrant
     * of the arena. Score is weighted by distance from center (closer = higher score).
     *
     * Safe zone: Center at (gridSize_X/4, gridSize_Y/4), radius = gridSize_X/4
     * Scoring: Linear interpolation from 1.0 at center to 0.0 at radius edge
     */
    case CHALLENGE_CIRCLE: {
      Coordinate safeCenter{(int16_t)(parameterMngrSingleton.gridSize_X / 4.0),
                            (int16_t)(parameterMngrSingleton.gridSize_Y / 4.0)};
      float radius = parameterMngrSingleton.gridSize_X / 4.0;

      Coordinate offset = safeCenter - indiv.loc;
      float distance = offset.length();
      return distance <= radius ? std::pair<bool, float>{true, (radius - distance) / radius}
                                : std::pair<bool, float>{false, 0.0};
    }

    /**
     * @brief CHALLENGE_RIGHT_HALF - Survive by being on the right side of the arena.
     *
     * Binary challenge: All individuals on the right half (x > gridSize_X/2) survive.
     * Scoring: 1.0 (pass) or 0.0 (fail), no gradient
     */
    case CHALLENGE_RIGHT_HALF:
      return indiv.loc.x > parameterMngrSingleton.gridSize_X / 2 ? std::pair<bool, float>{true, 1.0}
                                                                 : std::pair<bool, float>{false, 0.0};

    /**
     * @brief CHALLENGE_RIGHT_QUARTER - Survive by being on the rightmost quarter of the arena.
     *
     * Binary challenge: Individuals must be in the rightmost 25% of the arena
     * (x > gridSize_X/2 + gridSize_X/4, i.e., x > 3*gridSize_X/4).
     * Scoring: 1.0 (pass) or 0.0 (fail)
     */
    case CHALLENGE_RIGHT_QUARTER:
      return indiv.loc.x > parameterMngrSingleton.gridSize_X / 2 + parameterMngrSingleton.gridSize_X / 4
                 ? std::pair<bool, float>{true, 1.0}
                 : std::pair<bool, float>{false, 0.0};

    /**
     * @brief CHALLENGE_LEFT_EIGHTH - Survive by being on the leftmost eighth of the arena.
     *
     * Binary challenge: Individuals must be in the leftmost 12.5% of the arena (x < gridSize_X/8).
     * Scoring: 1.0 (pass) or 0.0 (fail)
     */
    case CHALLENGE_LEFT_EIGHTH:
      return indiv.loc.x < parameterMngrSingleton.gridSize_X / 8 ? std::pair<bool, float>{true, 1.0}
                                                                 : std::pair<bool, float>{false, 0.0};

    /**
     * @brief CHALLENGE_STRING - Survive by forming a "string" pattern with neighbors.
     *
     * Complex social challenge requiring individuals to:
     * 1. Not touch arena borders
     * 2. Have between minNeighbors and maxNeighbors (inclusive) within radius 1.5
     *
     * Current parameters: minNeighbors=2, maxNeighbors=22
     * This creates conditions favoring small clusters.
     *
     * Scoring: 1.0 (pass) or 0.0 (fail)
     */
    case CHALLENGE_STRING: {
      unsigned minNeighbors = 2;
      unsigned maxNeighbors = 22;
      float radius = 1.5;

      if (grid.isBorder(indiv.loc)) {
        return {false, 0.0};
      }

      unsigned count = 0;
      auto f = [&](Coordinate loc2) {
        if (grid.isOccupiedAt(loc2))
          ++count;
      };

      visitNeighborhood(indiv.loc, radius, f);
      if (count >= minNeighbors && count <= maxNeighbors) {
        return {true, 1.0};
      } else {
        return {false, 0.0};
      }
    }

    /**
     * @brief CHALLENGE_CENTER_WEIGHTED - Survive by being near the arena center (weighted scoring).
     *
     * Individuals within a circular zone centered in the arena survive. Score is linearly
     * weighted by distance from center (closer = higher score).
     *
     * Safe zone: Center at (gridSize_X/2, gridSize_Y/2), radius = gridSize_X/3
     * Scoring: Linear gradient from 1.0 at center to 0.0 at radius edge
     */
    case CHALLENGE_CENTER_WEIGHTED: {
      Coordinate safeCenter{(int16_t)(parameterMngrSingleton.gridSize_X / 2.0),
                            (int16_t)(parameterMngrSingleton.gridSize_Y / 2.0)};
      float radius = parameterMngrSingleton.gridSize_X / 3.0;

      Coordinate offset = safeCenter - indiv.loc;
      float distance = offset.length();
      return distance <= radius ? std::pair<bool, float>{true, (radius - distance) / radius}
                                : std::pair<bool, float>{false, 0.0};
    }

    /**
     * @brief CHALLENGE_CENTER_UNWEIGHTED - Survive by being near the arena center (binary scoring).
     *
     * Same spatial requirement as CENTER_WEIGHTED but with binary scoring.
     *
     * Safe zone: Center at (gridSize_X/2, gridSize_Y/2), radius = gridSize_X/3
     * Scoring: 1.0 (inside) or 0.0 (outside), no distance gradient
     */
    case CHALLENGE_CENTER_UNWEIGHTED: {
      Coordinate safeCenter{(int16_t)(parameterMngrSingleton.gridSize_X / 2.0),
                            (int16_t)(parameterMngrSingleton.gridSize_Y / 2.0)};
      float radius = parameterMngrSingleton.gridSize_X / 3.0;

      Coordinate offset = safeCenter - indiv.loc;
      float distance = offset.length();
      return distance <= radius ? std::pair<bool, float>{true, 1.0} : std::pair<bool, float>{false, 0.0};
    }

    /**
     * @brief CHALLENGE_CENTER_SPARSE - Survive by being near center with specific neighbor density.
     *
     * Complex spatial + social challenge combining:
     * 1. Location: Must be within outerRadius of arena center
     * 2. Social: Must have between minNeighbors and maxNeighbors (inclusive) within innerRadius
     *
     * Parameters:
     * - outerRadius = gridSize_X/4 (defines eligible zone)
     * - innerRadius = 1.5 (neighbor counting radius)
     * - minNeighbors = 5, maxNeighbors = 8 (includes self in count)
     *
     * Scoring: 1.0 (pass both criteria) or 0.0 (fail either)
     */
    case CHALLENGE_CENTER_SPARSE: {
      Coordinate safeCenter{(int16_t)(parameterMngrSingleton.gridSize_X / 2.0),
                            (int16_t)(parameterMngrSingleton.gridSize_Y / 2.0)};
      float outerRadius = parameterMngrSingleton.gridSize_X / 4.0;
      float innerRadius = 1.5;
      unsigned minNeighbors = 5;  ///< includes self
      unsigned maxNeighbors = 8;

      Coordinate offset = safeCenter - indiv.loc;
      float distance = offset.length();
      if (distance <= outerRadius) {
        unsigned count = 0;
        auto f = [&](Coordinate loc2) {
          if (grid.isOccupiedAt(loc2))
            ++count;
        };

        visitNeighborhood(indiv.loc, innerRadius, f);
        if (count >= minNeighbors && count <= maxNeighbors) {
          return {true, 1.0};
        }
      }
      return {false, 0.0};
    }

    /**
     * @brief CHALLENGE_CORNER - Survive by being near any corner (binary scoring).
     *
     * Individuals must be within a specified radius of any of the four arena corners.
     * Requires square arena (gridSize_X == gridSize_Y).
     *
     * Parameters: radius = gridSize_X/8
     * Corners tested: (0,0), (0,gridSize_Y-1), (gridSize_X-1,0), (gridSize_X-1,gridSize_Y-1)
     * Scoring: 1.0 (near any corner) or 0.0 (too far from all corners)
     */
    case CHALLENGE_CORNER: {
      assert(parameterMngrSingleton.gridSize_X == parameterMngrSingleton.gridSize_Y);
      float radius = parameterMngrSingleton.gridSize_X / 8.0;

      float distance = (Coordinate(0, 0) - indiv.loc).length();
      if (distance <= radius) {
        return {true, 1.0};
      }
      distance = (Coordinate(0, parameterMngrSingleton.gridSize_Y - 1) - indiv.loc).length();
      if (distance <= radius) {
        return {true, 1.0};
      }
      distance = (Coordinate(parameterMngrSingleton.gridSize_X - 1, 0) - indiv.loc).length();
      if (distance <= radius) {
        return {true, 1.0};
      }
      distance = (Coordinate(parameterMngrSingleton.gridSize_X - 1, parameterMngrSingleton.gridSize_Y - 1) - indiv.loc)
                     .length();
      if (distance <= radius) {
        return {true, 1.0};
      }
      return {false, 0.0};
    }

    /**
     * @brief CHALLENGE_CORNER_WEIGHTED - Survive by being near any corner (weighted scoring).
     *
     * Same corner-seeking behavior as CHALLENGE_CORNER but with distance-weighted scoring.
     * Requires square arena (gridSize_X == gridSize_Y).
     *
     * Parameters: radius = gridSize_X/4 (larger than unweighted version)
     * Scoring: Linear gradient from 1.0 at corner to 0.0 at radius edge, evaluated for nearest corner
     */
    case CHALLENGE_CORNER_WEIGHTED: {
      assert(parameterMngrSingleton.gridSize_X == parameterMngrSingleton.gridSize_Y);
      float radius = parameterMngrSingleton.gridSize_X / 4.0;

      float distance = (Coordinate(0, 0) - indiv.loc).length();
      if (distance <= radius) {
        return {true, (radius - distance) / radius};
      }
      distance = (Coordinate(0, parameterMngrSingleton.gridSize_Y - 1) - indiv.loc).length();
      if (distance <= radius) {
        return {true, (radius - distance) / radius};
      }
      distance = (Coordinate(parameterMngrSingleton.gridSize_X - 1, 0) - indiv.loc).length();
      if (distance <= radius) {
        return {true, (radius - distance) / radius};
      }
      distance = (Coordinate(parameterMngrSingleton.gridSize_X - 1, parameterMngrSingleton.gridSize_Y - 1) - indiv.loc)
                     .length();
      if (distance <= radius) {
        return {true, (radius - distance) / radius};
      }
      return {false, 0.0};
    }

    /**
     * @brief CHALLENGE_RADIOACTIVE_WALLS - Survival handled during simulation steps.
     *
     * This challenge is primarily evaluated in endOfSimStep(), where individuals
     * may die when touching walls during any simulation step (not just at generation end).
     * All individuals that survive to generation end become parents.
     *
     * Scoring: 1.0 (survived to generation end) or 0.0 (died during generation)
     * @note The actual death logic is in endOfSimStep(), not here
     */
    case CHALLENGE_RADIOACTIVE_WALLS:
      return {true, 1.0};

    /**
     * @brief CHALLENGE_AGAINST_ANY_WALL - Survive by touching any wall at generation end.
     *
     * Binary challenge: Individuals must be at an arena edge when generation ends.
     * Edge detection: x=0, x=gridSize_X-1, y=0, or y=gridSize_Y-1
     *
     * Scoring: 1.0 (on edge) or 0.0 (interior location)
     */
    case CHALLENGE_AGAINST_ANY_WALL: {
      bool onEdge = indiv.loc.x == 0 || indiv.loc.x == parameterMngrSingleton.gridSize_X - 1 || indiv.loc.y == 0 ||
                    indiv.loc.y == parameterMngrSingleton.gridSize_Y - 1;

      if (onEdge) {
        return {true, 1.0};
      } else {
        return {false, 0.0};
      }
    }

    /**
     * @brief CHALLENGE_TOUCH_ANY_WALL - Survive by touching any wall at least once during life.
     *
     * Temporal challenge: Individuals must touch a wall at ANY point during the generation,
     * not necessarily at the end. Wall contact is tracked in endOfSimStep() by setting
     * flags in indiv.challengeBits.
     *
     * Scoring: 1.0 (touched wall at any time) or 0.0 (never touched wall)
     * @note Requires coordination with endOfSimStep() for flag tracking
     */
    case CHALLENGE_TOUCH_ANY_WALL:
      if (indiv.challengeBits != 0) {
        return {true, 1.0};
      } else {
        return {false, 0.0};
      }

    /**
     * @brief CHALLENGE_MIGRATE_DISTANCE - All survive, scored by migration distance.
     *
     * Non-lethal challenge: All individuals survive regardless of performance.
     * Score is based on distance traveled from birth location to final location,
     * normalized by arena dimensions.
     *
     * Scoring: distance / max(gridSize_X, gridSize_Y), range [0.0, ~1.41] for diagonal travel
     * @note Higher scores favor individuals that migrated farther from birthplace
     */
    case CHALLENGE_MIGRATE_DISTANCE: {
      /// unsigned requiredDistance = p.sizeX / 2.0;
      float distance = (indiv.loc - indiv.birthLoc).length();
      distance = distance / (float)(std::max(parameterMngrSingleton.gridSize_X, parameterMngrSingleton.gridSize_Y));
      return {true, distance};
    }

    /**
     * @brief CHALLENGE_EAST_WEST_EIGHTHS - Survive by being on far left or far right.
     *
     * Binary challenge: Individuals must be in either:
     * - Leftmost eighth (x < gridSize_X/8), OR
     * - Rightmost eighth (x >= gridSize_X - gridSize_X/8)
     *
     * Scoring: 1.0 (in either edge zone) or 0.0 (in middle 75% of arena)
     */
    case CHALLENGE_EAST_WEST_EIGHTHS:
      return indiv.loc.x < parameterMngrSingleton.gridSize_X / 8 ||
                     indiv.loc.x >= (parameterMngrSingleton.gridSize_X - parameterMngrSingleton.gridSize_X / 8)
                 ? std::pair<bool, float>{true, 1.0}
                 : std::pair<bool, float>{false, 0.0};

    /**
     * @brief CHALLENGE_NEAR_BARRIER - Survive by being near any barrier (weighted scoring).
     *
     * Individuals must be within radius of any barrier center in the arena.
     * Score is inversely weighted by distance to nearest barrier (closer = higher score).
     *
     * Parameters: radius = gridSize_X/2 (current setting, alternatives commented in code)
     * Scoring: 1.0 - (distance/radius) for nearest barrier within radius, 0.0 if too far
     * @note Requires barriers to be placed in arena (see createBarrier.cpp)
     */
    case CHALLENGE_NEAR_BARRIER: {
      float radius;
      /// radius = 20.0;
      radius = parameterMngrSingleton.gridSize_X / 2;
      /// radius = p.sizeX / 4;

      const std::vector<Coordinate> barrierCenters = grid.getBarrierCenters();
      float minDistance = 1e8;
      for (auto& center : barrierCenters) {
        float distance = (indiv.loc - center).length();
        if (distance < minDistance) {
          minDistance = distance;
        }
      }
      if (minDistance <= radius) {
        return {true, 1.0 - (minDistance / radius)};
      } else {
        return {false, 0.0};
      }
    }

    /**
     * @brief CHALLENGE_PAIRS - Survive by forming an isolated pair with exactly one neighbor.
     *
     * Complex social challenge requiring:
     * 1. Individual must not touch arena border
     * 2. Individual must have exactly ONE neighbor (3x3 neighborhood, excluding self)
     * 3. That neighbor must have NO other neighbors (only the pair member)
     *
     * This creates isolated pairs in the arena with no other individuals nearby.
     *
     * Scoring: 1.0 (valid pair) or 0.0 (wrong neighbor count or neighbor has other neighbors)
     */
    case CHALLENGE_PAIRS: {
      bool onEdge = indiv.loc.x == 0 || indiv.loc.x == parameterMngrSingleton.gridSize_X - 1 || indiv.loc.y == 0 ||
                    indiv.loc.y == parameterMngrSingleton.gridSize_Y - 1;

      if (onEdge) {
        return {false, 0.0};
      }

      unsigned count = 0;
      for (int16_t x = indiv.loc.x - 1; x <= indiv.loc.x + 1; ++x) {
        for (int16_t y = indiv.loc.y - 1; y <= indiv.loc.y + 1; ++y) {
          Coordinate tloc(x, y);
          if (tloc != indiv.loc && grid.isInBounds(tloc) && grid.isOccupiedAt(tloc)) {
            ++count;
            if (count == 1) {
              for (int16_t x1 = tloc.x - 1; x1 <= tloc.x + 1; ++x1) {
                for (int16_t y1 = tloc.y - 1; y1 <= tloc.y + 1; ++y1) {
                  Coordinate tloc1(x1, y1);
                  if (tloc1 != tloc && tloc1 != indiv.loc && grid.isInBounds(tloc1) && grid.isOccupiedAt(tloc1)) {
                    return {false, 0.0};
                  }
                }
              }
            } else {
              return {false, 0.0};
            }
          }
        }
      }
      if (count == 1) {
        return {true, 1.0};
      } else {
        return {false, 0.0};
      }
    }

    /**
     * @brief CHALLENGE_LOCATION_SEQUENCE - Survive by visiting target locations in sequence.
     *
     * Temporal challenge: Individuals must contact specified locations during the generation.
     * Each location contacted sets a bit in indiv.challengeBits (tracked in endOfSimStep()).
     * Score reflects the number of locations successfully contacted.
     *
     * Scoring: (number of bits set) / (maximum possible bits), range [0.0, 1.0]
     *          Higher scores = more locations visited in sequence
     * @note Requires coordination with endOfSimStep() for bit flag tracking
     * @note Passes if at least one location was contacted (count > 0)
     */
    case CHALLENGE_LOCATION_SEQUENCE: {
      unsigned count = 0;
      unsigned bits = indiv.challengeBits;
      unsigned maxNumberOfBits = sizeof(bits) * 8;

      for (unsigned n = 0; n < maxNumberOfBits; ++n) {
        if ((bits & (1 << n)) != 0) {
          ++count;
        }
      }
      if (count > 0) {
        return {true, count / (float)maxNumberOfBits};
      } else {
        return {false, 0.0};
      }
    } break;

    /**
     * @brief CHALLENGE_ALTRUISM_SACRIFICE - Survive by gathering in NE quadrant (weighted).
     *
     * Individuals must be within a circular zone in the northeast quadrant.
     * Score is weighted by distance from center (closer = higher score).
     * Name suggests altruism mechanics but current implementation is pure location-based.
     *
     * Safe zone: Center at (3*gridSize_X/4, 3*gridSize_Y/4), radius = gridSize_X/4
     * In 128x128 arena: holds ~804 agents with current radius
     * Scoring: Linear gradient from 1.0 at center to 0.0 at radius edge
     */
    case CHALLENGE_ALTRUISM_SACRIFICE: {
      /// float radius = p.sizeX / 3.0; // in 128^2 world, holds 1429 agents
      float radius = parameterMngrSingleton.gridSize_X / 4.0;  ///< in 128^2 world, holds 804 agents
      /// float radius = p.sizeX / 5.0; // in 128^2 world, holds 514 agents

      float distance = (Coordinate(parameterMngrSingleton.gridSize_X - parameterMngrSingleton.gridSize_X / 4,
                                   parameterMngrSingleton.gridSize_Y - parameterMngrSingleton.gridSize_Y / 4) -
                        indiv.loc)
                           .length();
      if (distance <= radius) {
        return {true, (radius - distance) / radius};
      } else {
        return {false, 0.0};
      }
    }

    /**
     * @brief CHALLENGE_ALTRUISM - Survive by gathering in SW quadrant (weighted).
     *
     * Individuals must be within a circular zone in the southwest quadrant.
     * Score is weighted by distance from center (closer = higher score).
     * Name suggests altruism mechanics but current implementation is pure location-based.
     *
     * Safe zone: Center at (gridSize_X/4, gridSize_Y/4), radius = gridSize_X/4
     * In 128x128 arena: holds ~3216 agents (larger capacity than ALTRUISM_SACRIFICE)
     * Scoring: Linear gradient from 1.0 at center to 0.0 at radius edge
     */
    case CHALLENGE_ALTRUISM: {
      Coordinate safeCenter{(int16_t)(parameterMngrSingleton.gridSize_X / 4.0),
                            (int16_t)(parameterMngrSingleton.gridSize_Y / 4.0)};
      float radius = parameterMngrSingleton.gridSize_X / 4.0;  ///< in a 128^2 world, holds 3216

      Coordinate offset = safeCenter - indiv.loc;
      float distance = offset.length();
      return distance <= radius ? std::pair<bool, float>{true, (radius - distance) / radius}
                                : std::pair<bool, float>{false, 0.0};
    }

    default:
      assert(false);
  }
}

}  // namespace BioSim
