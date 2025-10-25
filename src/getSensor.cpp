/**
 * @file getSensor.cpp
 * @brief Sensor sampling helpers used by Individual::feedForward().
 *
 * Sensors convert raw world or internal state into normalized values within
 * `[SENSOR_MIN, SENSOR_MAX]`. Many sensors share helper utilities (population
 * density sampling, signal aggregation, probe-based distance queries, etc.)
 * which live in this translation unit.
 */

#include "simulator.h"

#include <cassert>
#include <cmath>
#include <iostream>

namespace BioSim {

/**
 * @brief Estimate directional population density for a neighborhood.
 * @param loc Center coordinate of the probe.
 * @param dir Axis along which density is projected (CENTER not allowed).
 * @return Normalized sensor value in `[0.0, 1.0]` where 0.5 means balanced load.
 *
 * @details
 * Every occupied neighbor contributes `proj / dist^2` where `proj` is the signed
 * projection of the offset vector onto the requested axis. Positive results
 * bias toward the forward direction whereas negative results indicate more
 * population behind the individual. The aggregate is normalized by an empiric
 * maximum (`~2 * radius`) and shifted into the sensor range.
 */
float getPopulationDensityAlongAxis(Coordinate loc, Dir dir) {
  assert(dir != Compass::CENTER);  ///< require a defined axis

  double sum = 0.0;
  Coordinate dirVec = dir.asNormalizedCoord();
  double len = std::sqrt(dirVec.x * dirVec.x + dirVec.y * dirVec.y);
  double dirVecX = dirVec.x / len;
  double dirVecY = dirVec.y / len;  ///< Unit vector components along dir

  auto f = [&](Coordinate tloc) {
    if (tloc != loc && grid.isOccupiedAt(tloc)) {
      Coordinate offset = tloc - loc;
      double proj = dirVecX * offset.x + dirVecY * offset.y;  ///< Magnitude of projection along dir
      double contrib = proj / (offset.x * offset.x + offset.y * offset.y);
      sum += contrib;
    }
  };

  visitNeighborhood(loc, parameterMngrSingleton.populationSensorRadius, f);

  double maxSumMag = 6.0 * parameterMngrSingleton.populationSensorRadius;
  assert(sum >= -maxSumMag && sum <= maxSumMag);

  double sensorVal;
  sensorVal = sum / maxSumMag;          ///< convert to -1.0..1.0
  sensorVal = (sensorVal + 1.0) / 2.0;  ///< convert to 0.0..1.0

  return sensorVal;
}

/**
 * @brief Short-range probe that compares barrier distances in opposite directions.
 * @param loc0 Starting coordinate (current individual location).
 * @param dir Axis to probe along (forward vs reverse).
 * @param probeDistance Maximum number of cells to inspect each way.
 * @return Normalized sensor value in `[0.0, 1.0]` (0.5 == balanced).
 *
 * The helper counts how many free cells exist before the first barrier (or
 * border) in both the forward and reverse direction. The differential distance
 * is mapped to sensor space, producing high values when the forward path is
 * clearer than the reverse.
 */
float getShortProbeBarrierDistance(Coordinate loc0, Dir dir, unsigned probeDistance) {
  unsigned countFwd = 0;
  unsigned countRev = 0;
  Coordinate loc = loc0 + dir;
  unsigned numLocsToTest = probeDistance;
  /// Scan positive direction
  while (numLocsToTest > 0 && grid.isInBounds(loc) && !grid.isBarrierAt(loc)) {
    ++countFwd;
    loc = loc + dir;
    --numLocsToTest;
  }
  if (numLocsToTest > 0 && !grid.isInBounds(loc)) {
    countFwd = probeDistance;
  }
  /// Scan negative direction
  numLocsToTest = probeDistance;
  loc = loc0 - dir;
  while (numLocsToTest > 0 && grid.isInBounds(loc) && !grid.isBarrierAt(loc)) {
    ++countRev;
    loc = loc - dir;
    --numLocsToTest;
  }
  if (numLocsToTest > 0 && !grid.isInBounds(loc)) {
    countRev = probeDistance;
  }

  float sensorVal = ((countFwd - countRev) + probeDistance);  ///< convert to 0..2*probeDistance
  sensorVal = (sensorVal / 2.0) / probeDistance;              ///< convert to 0.0..1.0
  return sensorVal;
}

/**
 * @brief Sample average pheromone magnitude around a coordinate.
 * @param layerNum Signal layer index.
 * @param loc Probe center.
 * @return Mean signal intensity normalized to `[0.0, 1.0]`.
 */
float getSignalDensity(unsigned layerNum, Coordinate loc) {
  unsigned countLocs = 0;
  unsigned long sum = 0;
  Coordinate center = loc;

  auto f = [&](Coordinate tloc) {
    ++countLocs;
    sum += pheromones.getMagnitude(layerNum, tloc);
  };

  visitNeighborhood(center, parameterMngrSingleton.signalSensorRadius, f);
  double maxSum = (float)countLocs * SIGNAL_MAX;
  double sensorVal = sum / maxSum;  ///< convert to 0.0..1.0

  return sensorVal;
}

/**
 * @brief Estimate directional pheromone density.
 * @param layerNum Signal layer index.
 * @param loc Probe center.
 * @param dir Axis along which signal strength is projected.
 * @return Normalized sensor value in `[0.0, 1.0]` (0.5 == balanced).
 *
 * Each neighbor contributes a weighted amount proportional to its signal
 * magnitude, the cosine of its angle relative to the axis, and the inverse
 * square of its distance. Negative accumulation indicates stronger signal
 * behind the agent, which maps to values below 0.5 after normalization.
 */
float getSignalDensityAlongAxis(unsigned layerNum, Coordinate loc, Dir dir) {
  assert(dir != Compass::CENTER);  ///< require a defined axis

  double sum = 0.0;
  Coordinate dirVec = dir.asNormalizedCoord();
  double len = std::sqrt(dirVec.x * dirVec.x + dirVec.y * dirVec.y);
  double dirVecX = dirVec.x / len;
  double dirVecY = dirVec.y / len;  ///< Unit vector components along dir

  auto f = [&](Coordinate tloc) {
    if (tloc != loc) {
      Coordinate offset = tloc - loc;
      double proj = (dirVecX * offset.x + dirVecY * offset.y);  ///< Magnitude of projection along dir
      double contrib = (proj * pheromones.getMagnitude(layerNum, tloc)) / (offset.x * offset.x + offset.y * offset.y);
      sum += contrib;
    }
  };

  visitNeighborhood(loc, parameterMngrSingleton.signalSensorRadius, f);

  double maxSumMag = 6.0 * parameterMngrSingleton.signalSensorRadius * SIGNAL_MAX;
  assert(sum >= -maxSumMag && sum <= maxSumMag);
  double sensorVal = sum / maxSumMag;   ///< convert to -1.0..1.0
  sensorVal = (sensorVal + 1.0) / 2.0;  ///< convert to 0.0..1.0

  return sensorVal;
}

/**
 * @brief Count how many empty cells exist before the next agent along an axis.
 * @param loc Starting coordinate.
 * @param dir Direction to march (normalized compass direction).
 * @param longProbeDist Maximum cells to inspect.
 * @return Cells to next agent, or `longProbeDist` if the path is blocked or escapes the arena.
 */
unsigned longProbePopulationFwd(Coordinate loc, Dir dir, unsigned longProbeDist) {
  assert(longProbeDist > 0);
  unsigned count = 0;
  loc = loc + dir;
  unsigned numLocsToTest = longProbeDist;
  while (numLocsToTest > 0 && grid.isInBounds(loc) && grid.isEmptyAt(loc)) {
    ++count;
    loc = loc + dir;
    --numLocsToTest;
  }
  if (numLocsToTest > 0 && (!grid.isInBounds(loc) || grid.isBarrierAt(loc))) {
    return longProbeDist;
  } else {
    return count;
  }
}

/**
 * @brief Count how many empty cells exist before the next barrier.
 * @param loc Starting coordinate.
 * @param dir Direction to march (normalized compass direction).
 * @param longProbeDist Maximum cells to inspect.
 * @return Cells to next barrier or `longProbeDist` if none encountered before bounds.
 *
 * Agents in the path do not interrupt the probe; only barriers or borders stop it.
 */
unsigned longProbeBarrierFwd(Coordinate loc, Dir dir, unsigned longProbeDist) {
  assert(longProbeDist > 0);
  unsigned count = 0;
  loc = loc + dir;
  unsigned numLocsToTest = longProbeDist;
  while (numLocsToTest > 0 && grid.isInBounds(loc) && !grid.isBarrierAt(loc)) {
    ++count;
    loc = loc + dir;
    --numLocsToTest;
  }
  if (numLocsToTest > 0 && !grid.isInBounds(loc)) {
    return longProbeDist;
  } else {
    return count;
  }
}

/**
 * @brief Resolve a sensor enum value into its normalized reading.
 * @param sensorNum Sensor to evaluate.
 * @param simStep Current simulation step (used for oscillators/time-varying probes).
 * @return Value clamped to `[SENSOR_MIN, SENSOR_MAX]`.
 *
 * @details
 * Most sensors query world state via helpers in this file (population density,
 * pheromones, barrier probes). Others simply transform local attributes such as
 * age or last movement direction. Every branch is responsible for clamping or
 * normalizing the final value before returning.
 */
float Individual::getSensor(Sensor sensorNum, unsigned simStep) const {
  float sensorVal = 0.0;

  switch (sensorNum) {
    case Sensor::AGE:
      /// Converts age (units of simSteps compared to life expectancy)
      /// linearly to normalized sensor range 0.0..1.0
      sensorVal = (float)age / parameterMngrSingleton.stepsPerGeneration;
      break;
    case Sensor::BOUNDARY_DIST: {
      /// Finds closest boundary, compares that to the max possible dist
      /// to a boundary from the center, and converts that linearly to the
      /// sensor range 0.0..1.0
      int distX = std::min<int>(loc.x, (parameterMngrSingleton.gridSize_X - loc.x) - 1);
      int distY = std::min<int>(loc.y, (parameterMngrSingleton.gridSize_Y - loc.y) - 1);
      int closest = std::min<int>(distX, distY);
      int maxPossible =
          std::max<int>(parameterMngrSingleton.gridSize_X / 2 - 1, parameterMngrSingleton.gridSize_Y / 2 - 1);
      sensorVal = (float)closest / maxPossible;
      break;
    }
    case Sensor::BOUNDARY_DIST_X: {
      /// Measures the distance to nearest boundary in the east-west axis,
      /// max distance is half the grid width; scaled to sensor range 0.0..1.0.
      int minDistX = std::min<int>(loc.x, (parameterMngrSingleton.gridSize_X - loc.x) - 1);
      sensorVal = minDistX / (parameterMngrSingleton.gridSize_X / 2.0);
      break;
    }
    case Sensor::BOUNDARY_DIST_Y: {
      /// Measures the distance to nearest boundary in the south-north axis,
      /// max distance is half the grid height; scaled to sensor range 0.0..1.0.
      int minDistY = std::min<int>(loc.y, (parameterMngrSingleton.gridSize_Y - loc.y) - 1);
      sensorVal = minDistY / (parameterMngrSingleton.gridSize_Y / 2.0);
      break;
    }
    case Sensor::LAST_MOVE_DIR_X: {
      /// X component -1,0,1 maps to sensor values 0.0, 0.5, 1.0
      auto lastX = lastMoveDir.asNormalizedCoord().x;
      sensorVal = lastX == 0 ? 0.5 : (lastX == -1 ? 0.0 : 1.0);
      break;
    }
    case Sensor::LAST_MOVE_DIR_Y: {
      /// Y component -1,0,1 maps to sensor values 0.0, 0.5, 1.0
      auto lastY = lastMoveDir.asNormalizedCoord().y;
      sensorVal = lastY == 0 ? 0.5 : (lastY == -1 ? 0.0 : 1.0);
      break;
    }
    case Sensor::LOC_X:
      /// Maps current X location 0..p.sizeX-1 to sensor range 0.0..1.0
      sensorVal = (float)loc.x / (parameterMngrSingleton.gridSize_X - 1);
      break;
    case Sensor::LOC_Y:
      /// Maps current Y location 0..p.sizeY-1 to sensor range 0.0..1.0
      sensorVal = (float)loc.y / (parameterMngrSingleton.gridSize_Y - 1);
      break;
    case Sensor::OSC1: {
      /// Maps the oscillator sine wave to sensor range 0.0..1.0;
      /// cycles starts at simStep 0 for everbody.
      float phase = (simStep % oscPeriod) / (float)oscPeriod;  ///< 0.0..1.0
      float factor = -std::cos(phase * 2.0f * 3.1415927f);
      assert(factor >= -1.0f && factor <= 1.0f);
      factor += 1.0f;  ///< convert to 0.0..2.0
      factor /= 2.0;   ///< convert to 0.0..1.0
      sensorVal = factor;
      /// Clip any round-off error
      sensorVal = std::min<float>(1.0, std::max<float>(0.0, sensorVal));
      break;
    }
    case Sensor::LONGPROBE_POP_FWD: {
      /// Measures the distance to the nearest other individual in the
      /// forward direction. If non found, returns the maximum sensor value.
      /// Maps the result to the sensor range 0.0..1.0.
      sensorVal = longProbePopulationFwd(loc, lastMoveDir, longProbeDist) / (float)longProbeDist;  ///< 0..1
      break;
    }
    case Sensor::LONGPROBE_BAR_FWD: {
      /// Measures the distance to the nearest barrier in the forward
      /// direction. If non found, returns the maximum sensor value.
      /// Maps the result to the sensor range 0.0..1.0.
      sensorVal = longProbeBarrierFwd(loc, lastMoveDir, longProbeDist) / (float)longProbeDist;  ///< 0..1
      break;
    }
    case Sensor::POPULATION: {
      /// Returns population density in neighborhood converted linearly from
      /// 0..100% to sensor range
      unsigned countLocs = 0;
      unsigned countOccupied = 0;
      Coordinate center = loc;

      auto f = [&](Coordinate tloc) {
        ++countLocs;
        if (grid.isOccupiedAt(tloc)) {
          ++countOccupied;
        }
      };

      visitNeighborhood(center, parameterMngrSingleton.populationSensorRadius, f);
      sensorVal = (float)countOccupied / countLocs;
      break;
    }
    case Sensor::POPULATION_FWD:
      /// Sense population density along axis of last movement direction, mapped
      /// to sensor range 0.0..1.0
      sensorVal = getPopulationDensityAlongAxis(loc, lastMoveDir);
      break;
    case Sensor::POPULATION_LR:
      /// Sense population density along an axis 90 degrees from last movement
      /// direction
      sensorVal = getPopulationDensityAlongAxis(loc, lastMoveDir.rotate90DegCW());
      break;
    case Sensor::BARRIER_FWD:
      /// Sense the nearest barrier along axis of last movement direction, mapped
      /// to sensor range 0.0..1.0
      sensorVal = getShortProbeBarrierDistance(loc, lastMoveDir, parameterMngrSingleton.shortProbeBarrierDistance);
      break;
    case Sensor::BARRIER_LR:
      /// Sense the nearest barrier along axis perpendicular to last movement
      /// direction, mapped to sensor range 0.0..1.0
      sensorVal = getShortProbeBarrierDistance(loc, lastMoveDir.rotate90DegCW(),
                                               parameterMngrSingleton.shortProbeBarrierDistance);
      break;
    case Sensor::RANDOM:
      /// Returns a random sensor value in the range 0.0..1.0.
      sensorVal = randomUint() / (float)UINT_MAX;
      break;
    case Sensor::SIGNAL0:
      /// Returns magnitude of signal0 in the local neighborhood, with
      /// 0.0..maxSignalSum converted to sensorRange 0.0..1.0
      sensorVal = getSignalDensity(0, loc);
      break;
    case Sensor::SIGNAL0_FWD:
      /// Sense signal0 density along axis of last movement direction
      sensorVal = getSignalDensityAlongAxis(0, loc, lastMoveDir);
      break;
    case Sensor::SIGNAL0_LR:
      /// Sense signal0 density along an axis perpendicular to last movement
      /// direction
      sensorVal = getSignalDensityAlongAxis(0, loc, lastMoveDir.rotate90DegCW());
      break;
    case Sensor::GENETIC_SIM_FWD: {
      /// Return minimum sensor value if nobody is alive in the forward adjacent
      /// location, else returns a similarity match in the sensor range 0.0..1.0
      Coordinate loc2 = loc + lastMoveDir;
      if (grid.isInBounds(loc2) && grid.isOccupiedAt(loc2)) {
        const Individual& indiv2 = peeps.getIndiv(loc2);
        if (indiv2.alive) {
          sensorVal = genomeSimilarity(genome, indiv2.genome);  ///< 0.0..1.0
        }
      }
      break;
    }
    default:
      assert(false);
      break;
  }

  if (std::isnan(sensorVal) || sensorVal < -0.01 || sensorVal > 1.01) {
    std::cout << "sensorVal=" << (int)sensorVal << " for " << sensorName((Sensor)sensorNum) << std::endl;
    sensorVal = std::max(0.0f, std::min(sensorVal, 1.0f));  ///< clip
  }

  assert(!std::isnan(sensorVal) && sensorVal >= -0.01 && sensorVal <= 1.01);

  return sensorVal;
}

}  // namespace BioSim
