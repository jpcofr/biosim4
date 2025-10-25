/**
 * @file executeActions.cpp
 * @brief Interpret neural action levels and apply side effects to an individual.
 *
 * Each simulation step evaluates an individual's neural network (see
 * `feedForward.cpp`) to obtain raw action levels. This module translates those
 * arbitrary floating point values into concrete behavioral effects such as
 * movement, responsiveness, signaling, reproduction, or aggression. All
 * side-effects that mutate shared state are funneled through thread-safe
 * helpers (e.g., queues in `Peeps` or atomic grid writes) so the function can
 * be executed concurrently for every living individual.
 */

#include "omp.h"
#include "simulator.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <iostream>

namespace BioSim {

/**
 * @brief Convert a probability scalar into a Bernoulli trial.
 * @param factor Probability in `[0.0, 1.0]`.
 * @return true with likelihood equal to `factor`.
 */
bool prob2bool(float factor) {
  assert(factor >= 0.0 && factor <= 1.0);
  return (randomUint() / (float)RANDOM_UINT_MAX) < factor;
}

/**
 * @brief Apply the responsiveness shaping curve configured in parameters.
 *
 * The simulator uses a tunable exponential curve to desaturate overly
 * sensitive agents. The K factor (stored in parameters) governs steepness;
 * larger values require stronger action activations to trigger behavior.
 *
 * @param r Raw responsiveness in `[0.0, 1.0]`.
 * @return Adjusted responsiveness in `[0.0, 1.0]`.
 */
float responseCurve(float r) {
  const float k = parameterMngrSingleton.responsivenessCurveKFactor;
  return std::pow((r - 2.0), -2.0 * k) - std::pow(2.0, -2.0 * k) * (1.0 - r);
}

/**
 * @brief Translate neural action levels into changes for one individual.
 * @param indiv Individual to mutate.
 * @param actionLevels Raw outputs from `feedForward()`; arbitrary ranges.
 *
 * @details
 * **Execution model**
 * - The function runs concurrently for every living agent. Any mutation that
 *   affects shared world state (grid occupancy, death, movement) is therefore
 *   deferred via queues owned by `Peeps`, which are drained after the
 *   multi-threaded phase completes.
 * - Non-shared state (e.g., oscillator period) is updated immediately on the
 *   individual because each thread has exclusive ownership of its `indiv`.
 *
 * **Action families**
 * - `SET_*` actions alter latent properties such as responsiveness or
 *   oscillator period.
 * - `EMIT_SIGNAL*` actions deposit pheromones into the global signal fields.
 * - `KILL_FORWARD` attempts to remove the neighboring agent directly ahead,
 *   subject to probabilistic gating.
 * - Movement actions (`MOVE_*`, `MOVE_FORWARD`, `MOVE_RANDOM`, etc.) are
 *   combined vectorially, shaped by responsiveness, and converted into a
 *   normalized grid offset before being queued.
 *
 * The probabilities derive from the hyperbolic tangent of accumulated inputs,
 * which produces smooth transitions while keeping activations bounded.
 */
void executeActions(Individual& indiv, std::array<float, Action::NUM_ACTIONS>& actionLevels) {
  /// Only a subset of all possible actions might be enabled (i.e., compiled in).
  /// This returns true if the specified action is enabled. See sensors-actions.h
  /// for how to enable sensors and actions during compilation.
  auto isEnabled = [](enum Action action) { return (int)action < (int)Action::NUM_ACTIONS; };

  /// Responsiveness action - convert neuron action level from arbitrary float
  /// range to the range 0.0..1.0. If this action neuron is enabled but not
  /// driven, will default to mid-level 0.5.
  if (isEnabled(Action::SET_RESPONSIVENESS)) {
    float level = actionLevels[Action::SET_RESPONSIVENESS];  ///< default 0.0
    level = (std::tanh(level) + 1.0) / 2.0;                  ///< convert to 0.0..1.0
    indiv.responsiveness = level;
  }

  /// For the rest of the action outputs, we'll apply an adjusted responsiveness
  /// factor (see responseCurve() for more info). Range 0.0..1.0.
  float responsivenessAdjusted = responseCurve(indiv.responsiveness);

  /// Oscillator period action - convert action level nonlinearly to
  /// 2..4*p.stepsPerGeneration. If this action neuron is enabled but not driven,
  /// will default to 1.5 + e^(3.5) = a period of 34 simSteps.
  if (isEnabled(Action::SET_OSCILLATOR_PERIOD)) {
    auto periodf = actionLevels[Action::SET_OSCILLATOR_PERIOD];
    float newPeriodf01 = (std::tanh(periodf) + 1.0) / 2.0;  ///< convert to 0.0..1.0
    unsigned newPeriod = 1 + (int)(1.5 + std::exp(7.0 * newPeriodf01));
    assert(newPeriod >= 2 && newPeriod <= 2048);
    indiv.oscPeriod = newPeriod;
  }

  /// Set longProbeDistance - convert action level to 1..maxLongProbeDistance.
  /// If this action neuron is enabled but not driven, will default to
  /// mid-level period of 17 simSteps.
  if (isEnabled(Action::SET_LONGPROBE_DIST)) {
    constexpr unsigned maxLongProbeDistance = 32;
    float level = actionLevels[SET_LONGPROBE_DIST];
    level = (std::tanh(level) + 1.0) / 2.0;  ///< convert to 0.0..1.0
    level = 1 + level * maxLongProbeDistance;
    indiv.longProbeDist = (unsigned)level;
  }

  /// Emit signal0 - if this action value is below a threshold, nothing emitted.
  /// Otherwise convert the action value to a probability of emitting one unit of
  /// signal (pheromone).
  /// Pheromones may be emitted immediately (see signals.cpp). If this action
  /// neuron is enabled but not driven, nothing will be emitted.
  if (isEnabled(Action::EMIT_SIGNAL0)) {
    constexpr float emitThreshold = 0.5;  ///< 0.0..1.0; 0.5 is midlevel
    float level = actionLevels[Action::EMIT_SIGNAL0];
    level = (std::tanh(level) + 1.0) / 2.0;  ///< convert to 0.0..1.0
    level *= responsivenessAdjusted;
    if (level > emitThreshold && prob2bool(level)) {
      pheromones.increment(0, indiv.loc);
    }
  }

  /// Kill forward -- if this action value is > threshold, value is converted to
  /// probability of an attempted murder. Probabilities under the threshold are
  /// considered 0.0. If this action neuron is enabled but not driven, the
  /// neighbors are safe.
  if (isEnabled(Action::KILL_FORWARD) && parameterMngrSingleton.killEnable) {
    constexpr float killThreshold = 0.5;  ///< 0.0..1.0; 0.5 is midlevel
    float level = actionLevels[Action::KILL_FORWARD];
    level = (std::tanh(level) + 1.0) / 2.0;  ///< convert to 0.0..1.0
    level *= responsivenessAdjusted;
    if (level > killThreshold && prob2bool((level - ACTION_MIN) / ACTION_RANGE)) {
      Coordinate otherLoc = indiv.loc + indiv.lastMoveDir;
      if (grid.isInBounds(otherLoc) && grid.isOccupiedAt(otherLoc)) {
        Individual& indiv2 = peeps.getIndiv(otherLoc);
        assert((indiv.loc - indiv2.loc).length() == 1);
        peeps.queueForDeath(indiv2);
      }
    }
  }

  /// ------------- Movement action neurons ---------------

  /// There are multiple action neurons for movement. Each type of movement
  /// neuron urges the individual to move in some specific direction. We sum up
  /// all the X and Y components of all the movement urges, then pass the X and Y
  /// sums through a transfer function (tanh()) to get a range -1.0..1.0. The
  /// absolute values of the X and Y values are passed through prob2bool() to
  /// convert to -1, 0, or 1, then multiplied by the component's signum. This
  /// results in the x and y components of a normalized movement offset. I.e.,
  /// the probability of movement in either dimension is the absolute value of
  /// tanh of the action level X,Y components and the direction is the sign of
  /// the X, Y components. For example, for a particular action neuron:
  ///     X, Y == -5.9, +0.3 as raw action levels received here
  ///     X, Y == -0.999, +0.29 after passing raw values through tanh()
  ///     Xprob, Yprob == 99.9%, 29% probability of X and Y becoming 1 (or -1)
  ///     X, Y == -1, 0 after applying the sign and probability
  ///     The agent will then be moved West (an offset of -1, 0) if it's a legal
  ///     move.

  float level;
  Coordinate offset;
  Coordinate lastMoveOffset = indiv.lastMoveDir.asNormalizedCoord();

  /// moveX,moveY will be the accumulators that will hold the sum of all the
  /// urges to move along each axis. (+- floating values of arbitrary range)
  float moveX = isEnabled(Action::MOVE_X) ? actionLevels[Action::MOVE_X] : 0.0;
  float moveY = isEnabled(Action::MOVE_Y) ? actionLevels[Action::MOVE_Y] : 0.0;

  if (isEnabled(Action::MOVE_EAST))
    moveX += actionLevels[Action::MOVE_EAST];
  if (isEnabled(Action::MOVE_WEST))
    moveX -= actionLevels[Action::MOVE_WEST];
  if (isEnabled(Action::MOVE_NORTH))
    moveY += actionLevels[Action::MOVE_NORTH];
  if (isEnabled(Action::MOVE_SOUTH))
    moveY -= actionLevels[Action::MOVE_SOUTH];

  if (isEnabled(Action::MOVE_FORWARD)) {
    level = actionLevels[Action::MOVE_FORWARD];
    moveX += lastMoveOffset.x * level;
    moveY += lastMoveOffset.y * level;
  }
  if (isEnabled(Action::MOVE_REVERSE)) {
    level = actionLevels[Action::MOVE_REVERSE];
    moveX -= lastMoveOffset.x * level;
    moveY -= lastMoveOffset.y * level;
  }
  if (isEnabled(Action::MOVE_LEFT)) {
    level = actionLevels[Action::MOVE_LEFT];
    offset = indiv.lastMoveDir.rotate90DegCCW().asNormalizedCoord();
    moveX += offset.x * level;
    moveY += offset.y * level;
  }
  if (isEnabled(Action::MOVE_RIGHT)) {
    level = actionLevels[Action::MOVE_RIGHT];
    offset = indiv.lastMoveDir.rotate90DegCW().asNormalizedCoord();
    moveX += offset.x * level;
    moveY += offset.y * level;
  }
  if (isEnabled(Action::MOVE_RL)) {
    level = actionLevels[Action::MOVE_RL];
    offset = indiv.lastMoveDir.rotate90DegCW().asNormalizedCoord();
    moveX += offset.x * level;
    moveY += offset.y * level;
  }

  if (isEnabled(Action::MOVE_RANDOM)) {
    level = actionLevels[Action::MOVE_RANDOM];
    offset = Dir::random8().asNormalizedCoord();
    moveX += offset.x * level;
    moveY += offset.y * level;
  }

  /// Convert the accumulated X, Y sums to the range -1.0..1.0 and scale by the
  /// individual's responsiveness (0.0..1.0) (adjusted by a curve)
  moveX = std::tanh(moveX);
  moveY = std::tanh(moveY);
  moveX *= responsivenessAdjusted;
  moveY *= responsivenessAdjusted;

  /// The probability of movement along each axis is the absolute value
  int16_t probX = (int16_t)prob2bool(std::abs(moveX));  ///< convert abs(level) to 0 or 1
  int16_t probY = (int16_t)prob2bool(std::abs(moveY));  ///< convert abs(level) to 0 or 1

  /// The direction of movement (if any) along each axis is the sign
  int16_t signumX = moveX < 0.0 ? -1 : 1;
  int16_t signumY = moveY < 0.0 ? -1 : 1;

  /// Generate a normalized movement offset, where each component is -1, 0, or 1
  Coordinate movementOffset = {(int16_t)(probX * signumX), (int16_t)(probY * signumY)};

  /// Move there if it's a valid location
  Coordinate newLoc = indiv.loc + movementOffset;
  if (grid.isInBounds(newLoc) && grid.isEmptyAt(newLoc)) {
    peeps.queueForMove(indiv, newLoc);
  }
}

}  // namespace BioSim
