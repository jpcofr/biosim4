/**
 * @file executeActions.cpp
 * @brief Action execution and behavioral translation layer
 *
 * @section overview Overview
 * This module translates raw neural network outputs into concrete behavioral
 * effects for simulated individuals. It serves as the bridge between the
 * abstract neural computation layer (feedForward.cpp) and the physical world
 * state (grid, signals, individual properties).
 *
 * @section execution_model Execution Model
 * The primary function executeActions() is designed for concurrent execution:
 * - Called in parallel for all living individuals via OpenMP
 * - Thread-safe through deferred mutation queues (movement, death)
 * - Direct mutation only for thread-local individual properties
 * - Queues are drained in single-threaded context after parallel phase
 *
 * @section action_types Action Types
 * Actions are organized into functional families:
 * - **Property Setters** (SET_*): Modify internal states (responsiveness,
 *   oscillator period, probe distance)
 * - **Communication** (EMIT_SIGNAL*): Deposit pheromones into signal layers
 * - **Aggression** (KILL_FORWARD): Probabilistic neighbor elimination
 * - **Movement** (MOVE_*): Vectorial combination of directional urges
 *
 * @section value_mapping Value Mapping Strategy
 * Raw neural outputs (arbitrary float range) undergo standardized transformation:
 * 1. Apply hyperbolic tangent (tanh) to bound values to [-1, 1]
 * 2. Shift and scale to target range (typically [0, 1])
 * 3. Apply responsiveness curve for behavioral dampening
 * 4. Convert to probabilities via prob2bool() for stochastic execution
 *
 * @see feedForward.cpp for neural network evaluation
 * @see peeps.h for deferred queue mechanisms
 * @see sensors-actions.h for action enumeration and compilation settings
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
 * @brief Convert probability scalar to boolean outcome (Bernoulli trial)
 *
 * Performs a single Bernoulli trial by comparing a uniform random value
 * against the input probability factor. Used throughout action execution
 * to convert continuous neural outputs into discrete behavioral decisions.
 *
 * @param factor Probability of returning true, must be in range [0.0, 1.0]
 * @return true with probability equal to factor, false otherwise
 *
 * @pre factor must satisfy: 0.0 <= factor <= 1.0 (asserted)
 * @post Return value is true with probability exactly equal to factor
 *
 * @note Uses global randomUint() function for uniform random generation
 * @note Thread-safe if randomUint() is thread-safe (implementation dependent)
 *
 * @warning Violating the precondition triggers assertion failure
 */
bool prob2bool(float factor) {
  assert(factor >= 0.0 && factor <= 1.0);
  return (randomUint() / (float)RANDOM_UINT_MAX) < factor;
}

/**
 * @brief Apply responsiveness shaping curve to dampen neural sensitivity
 *
 * Transforms raw responsiveness values through a configurable exponential
 * curve to prevent oversaturation of agent responses. This non-linear
 * transformation allows fine control over how strongly individuals react
 * to their neural network outputs.
 *
 * The curve is controlled by the K factor parameter (responsivenessCurveKFactor):
 * - **Higher K values**: Require stronger neural activations to trigger actions
 *   (steeper curve, more dampening)
 * - **Lower K values**: More sensitive responses to neural outputs (gentler curve)
 *
 * @param r Raw responsiveness value, typically in range [0.0, 1.0]
 * @return Adjusted responsiveness value in range [0.0, 1.0]
 *
 * @note The mathematical form is: (r - 2)^(-2k) - 2^(-2k) * (1 - r)
 * @note K factor is retrieved from global parameterMngrSingleton
 * @note This transformation is applied before most action executions
 *
 * @see parameterMngrSingleton.responsivenessCurveKFactor
 * @see executeActions() for usage in action probability calculations
 */
float responseCurve(float r) {
  const float k = parameterMngrSingleton.responsivenessCurveKFactor;
  return std::pow((r - 2.0), -2.0 * k) - std::pow(2.0, -2.0 * k) * (1.0 - r);
}

/**
 * @brief Execute all enabled actions for an individual based on neural outputs
 *
 * This is the primary behavioral translation function, called once per individual
 * per simulation step. It processes raw neural network action levels and converts
 * them into concrete world effects (movement, signaling, property changes, etc.).
 *
 * @param[in,out] indiv Individual whose actions are being executed
 *                      - Properties may be modified (responsiveness, oscPeriod, etc.)
 *                      - Movement/death are queued, not applied immediately
 * @param[in] actionLevels Array of raw neural outputs, one per action type
 *                         - Values have arbitrary float range from neural network
 *                         - Indexed by Action enum values
 *                         - Only NUM_ACTIONS entries are valid (see sensors-actions.h)
 *
 * @section threading Threading Safety
 * This function is designed for concurrent execution via OpenMP:
 * - **Thread-safe operations**: Direct mutation of individual properties
 *   (responsiveness, oscPeriod, longProbeDist) - each thread owns one individual
 * - **Deferred operations**: Movement and death are queued via thread-safe
 *   Peeps methods (queueForMove, queueForDeath) and executed later in
 *   single-threaded context
 * - **Signal deposits**: Pheromone emissions use atomic increments internally
 *
 * @section action_processing Action Processing Order
 * Actions are processed in functional groups:
 *
 * 1. **SET_RESPONSIVENESS**: Sets behavioral sensitivity for subsequent actions
 *    - Converts neural output to [0, 1] via tanh normalization
 *    - Default value (no drive): 0.5 (mid-level responsiveness)
 *
 * 2. **SET_OSCILLATOR_PERIOD**: Configures internal timing cycle
 *    - Maps neural output to period range [2, 2048] steps exponentially
 *    - Default value (no drive): ~34 steps (1.5 + e^3.5)
 *    - Used by oscillator-based sensors (e.g., AGE_OSCILLATOR)
 *
 * 3. **SET_LONGPROBE_DIST**: Sets sensory probe distance
 *    - Maps neural output to range [1, 32] grid units
 *    - Default value (no drive): ~17 units (mid-range)
 *    - Affects long-distance sensor queries
 *
 * 4. **EMIT_SIGNAL0**: Probabilistic pheromone emission
 *    - Threshold-gated: must exceed 0.5 after normalization
 *    - Probability scaled by adjusted responsiveness
 *    - Deposits single unit to signal layer 0 at individual's location
 *
 * 5. **KILL_FORWARD**: Aggressive neighbor elimination
 *    - Threshold-gated: must exceed 0.5 after normalization
 *    - Requires killEnable parameter to be true
 *    - Target: neighbor in lastMoveDir direction
 *    - Probability scaled by adjusted responsiveness
 *    - Target queued for death if occupied and in bounds
 *
 * 6. **Movement Actions**: Vectorial combination (see detailed section below)
 *
 * @section movement_mechanics Movement Mechanics
 * Multiple movement action neurons can activate simultaneously. Their effects
 * combine vectorially through the following algorithm:
 *
 * **Step 1: Accumulation**
 * - Each active movement neuron contributes to moveX and moveY accumulators
 * - Directional actions (MOVE_EAST/WEST/NORTH/SOUTH): Add/subtract unit vectors
 * - Relative actions (MOVE_FORWARD/REVERSE/LEFT/RIGHT): Use lastMoveDir reference
 * - Abstract actions (MOVE_X/MOVE_Y): Direct neural output contribution
 * - Stochastic action (MOVE_RANDOM): Random direction contribution
 *
 * **Step 2: Normalization**
 * - Apply tanh() to bound moveX and moveY to [-1, 1]
 * - Scale by adjusted responsiveness curve
 *
 * **Step 3: Probabilistic Discretization**
 * - Absolute value determines movement probability for each axis
 * - prob2bool() converts probability to discrete {0, 1}
 * - Sign determines direction {-1, +1}
 * - Result: movement offset with components in {-1, 0, +1}
 *
 * **Step 4: Validation and Queuing**
 * - Calculate target location (current + offset)
 * - Queue move if target is in-bounds and unoccupied
 * - Invalid moves are silently discarded (no penalty)
 *
 * @par Movement Example:
 * @code
 * // Neural outputs:
 * actionLevels[MOVE_X] = -5.9
 * actionLevels[MOVE_Y] = +0.3
 *
 * // After tanh normalization:
 * moveX = tanh(-5.9) = -0.999
 * moveY = tanh(+0.3) = +0.29
 *
 * // Probability conversion:
 * probX = prob2bool(0.999) → 99.9% chance of 1
 * probY = prob2bool(0.29)  → 29% chance of 1
 *
 * // Example outcome: probX=1, probY=0
 * // With signs: offset = (-1, 0)
 * // Effect: Individual moves West if cell is empty
 * @endcode
 *
 * @note Action enablement is compile-time configured via sensors-actions.h
 *       enum ordering. Only actions before NUM_ACTIONS marker are processed.
 *
 * @see feedForward() for neural network evaluation
 * @see responseCurve() for responsiveness transformation
 * @see prob2bool() for probabilistic conversion
 * @see Peeps::queueForMove() for deferred movement
 * @see Peeps::queueForDeath() for deferred elimination
 * @see Signals::increment() for pheromone deposition
 * @see sensors-actions.h for action enumeration and compilation settings
 */
void executeActions(Individual& indiv, std::array<float, Action::NUM_ACTIONS>& actionLevels) {
  /**
   * @brief Lambda to check if an action is compiled into the executable
   *
   * Actions are selectively enabled at compile-time via enum ordering in
   * sensors-actions.h. Only actions appearing before the NUM_ACTIONS marker
   * are included in the build.
   *
   * @param action Action enum value to check
   * @return true if action is enabled (compiled in), false otherwise
   *
   * @note This is a compile-time pattern that affects functionality.
   *       Reordering enums in sensors-actions.h changes which actions execute.
   * @warning This pattern is documented as "fragile" in project TODO (#4)
   */
  auto isEnabled = [](enum Action action) { return (int)action < (int)Action::NUM_ACTIONS; };

  // ============================================================================
  // ACTION: SET_RESPONSIVENESS
  // ============================================================================
  /**
   * Sets the individual's behavioral sensitivity for all subsequent actions
   * in this step. Responsiveness acts as a global scaling factor that dampens
   * or amplifies reactions to neural outputs.
   *
   * **Value mapping**:
   * - Input: Arbitrary float from neural network (actionLevels)
   * - Transform: tanh() to bound to [-1, 1]
   * - Shift: Add 1.0 and divide by 2.0 → [0, 1]
   * - Default behavior: If neuron not driven, level = 0.0 → normalized to 0.5
   *
   * **Effect**: Updates indiv.responsiveness, which is then passed through
   * responseCurve() and applied as a multiplier to most other actions.
   */
  if (isEnabled(Action::SET_RESPONSIVENESS)) {
    float level = actionLevels[Action::SET_RESPONSIVENESS];  ///< Raw neural output (default: 0.0)
    level = (std::tanh(level) + 1.0) / 2.0;                  ///< Normalize to [0.0, 1.0]
    indiv.responsiveness = level;
  }

  /**
   * Calculate adjusted responsiveness using shaping curve.
   * This value is applied as a scaling factor to most action probabilities
   * to prevent oversaturation and create more nuanced behavioral responses.
   *
   * @see responseCurve() for curve mathematics
   */
  float responsivenessAdjusted = responseCurve(indiv.responsiveness);

  // ============================================================================
  // ACTION: SET_OSCILLATOR_PERIOD
  // ============================================================================
  /**
   * Configures the period of the individual's internal oscillator, used by
   * oscillator-based sensors (e.g., AGE_OSCILLATOR sensor).
   *
   * **Value mapping**:
   * - Input: Arbitrary float from neural network
   * - Transform: tanh() to [-1, 1], then normalize to [0, 1]
   * - Map: 1 + (int)(1.5 + e^(7.0 * normalized)) → [2, 2048] range
   * - Exponential mapping creates logarithmic sensitivity
   * - Default behavior: level = 0.0 → normalized = 0.5 → period ≈ 34 steps
   *
   * **Valid range**: [2, 2048] simulation steps (asserted)
   *
   * **Effect**: Updates indiv.oscPeriod, which modulates oscillator sensor outputs
   */
  if (isEnabled(Action::SET_OSCILLATOR_PERIOD)) {
    auto periodf = actionLevels[Action::SET_OSCILLATOR_PERIOD];
    float newPeriodf01 = (std::tanh(periodf) + 1.0) / 2.0;  ///< Normalize to [0.0, 1.0]
    unsigned newPeriod = 1 + (int)(1.5 + std::exp(7.0 * newPeriodf01));
    assert(newPeriod >= 2 && newPeriod <= 2048);
    indiv.oscPeriod = newPeriod;
  }

  // ============================================================================
  // ACTION: SET_LONGPROBE_DIST
  // ============================================================================
  /**
   * Sets maximum distance for long-range sensory probes (e.g., population
   * density sensors, distant wall detection).
   *
   * **Value mapping**:
   * - Input: Arbitrary float from neural network
   * - Transform: tanh() to [-1, 1], then normalize to [0, 1]
   * - Map: 1 + (normalized * maxLongProbeDistance) → [1, 32] grid units
   * - Linear mapping for intuitive distance control
   * - Default behavior: level = 0.0 → normalized = 0.5 → distance ≈ 17 units
   *
   * **Valid range**: [1, 32] grid units (maxLongProbeDistance = 32)
   *
   * **Effect**: Updates indiv.longProbeDist, which limits sensor query radius
   */
  if (isEnabled(Action::SET_LONGPROBE_DIST)) {
    constexpr unsigned maxLongProbeDistance = 32;  ///< Maximum sensory range
    float level = actionLevels[SET_LONGPROBE_DIST];
    level = (std::tanh(level) + 1.0) / 2.0;  ///< Normalize to [0.0, 1.0]
    level = 1 + level * maxLongProbeDistance;
    indiv.longProbeDist = (unsigned)level;
  }

  // ============================================================================
  // ACTION: EMIT_SIGNAL0
  // ============================================================================
  /**
   * Probabilistically deposits one unit of pheromone (signal) at the
   * individual's current location.
   *
   * **Value mapping**:
   * - Input: Arbitrary float from neural network
   * - Transform: tanh() to [-1, 1], then normalize to [0, 1]
   * - Scale: Multiply by adjusted responsiveness
   * - Gate: Activation only if level > emitThreshold (0.5)
   * - Probability: Final scaled level determines emission chance
   * - Default behavior: level = 0.0 → normalized = 0.5 → at threshold (50% chance)
   *
   * **Execution**:
   * 1. Check if normalized level exceeds threshold (0.5)
   * 2. If yes, roll probability dice (prob2bool)
   * 3. If success, deposit 1 unit to signal layer 0
   *
   * **Effect**: Increments pheromones at indiv.loc (thread-safe atomic operation)
   *
   * @note Signal layer 0 is hardcoded; future versions may support multiple layers
   * @see Signals::increment() for atomic pheromone deposition
   * @see signals.cpp for signal diffusion and decay mechanics
   */
  if (isEnabled(Action::EMIT_SIGNAL0)) {
    constexpr float emitThreshold = 0.5;  ///< Activation threshold [0.0, 1.0]; 0.5 is midlevel
    float level = actionLevels[Action::EMIT_SIGNAL0];
    level = (std::tanh(level) + 1.0) / 2.0;  ///< Normalize to [0.0, 1.0]
    level *= responsivenessAdjusted;
    if (level > emitThreshold && prob2bool(level)) {
      pheromones.increment(0, indiv.loc);
    }
  }

  // ============================================================================
  // ACTION: KILL_FORWARD
  // ============================================================================
  /**
   * Attempts to eliminate the neighboring individual in the direction of
   * last movement. This aggressive action is gated by both threshold and
   * global parameter settings.
   *
   * **Value mapping**:
   * - Input: Arbitrary float from neural network
   * - Transform: tanh() to [-1, 1], then normalize to [0, 1]
   * - Scale: Multiply by adjusted responsiveness
   * - Gate 1: Activation only if level > killThreshold (0.5)
   * - Gate 2: Must have killEnable parameter set to true
   * - Probability: (level - ACTION_MIN) / ACTION_RANGE determines success chance
   * - Default behavior: level = 0.0 → normalized = 0.5 → at threshold (marginal)
   *
   * **Target selection**:
   * - Direction: indiv.lastMoveDir (direction of most recent movement)
   * - Location: indiv.loc + lastMoveDir offset
   * - Requirements: Target must be in bounds and occupied
   *
   * **Execution**:
   * 1. Check threshold and global kill-enable flag
   * 2. Calculate target location based on last move direction
   * 3. Verify target is in-bounds and occupied
   * 4. Roll probability dice
   * 5. If success, queue target individual for death
   *
   * **Effect**: Queues neighbor for removal (deferred to single-threaded phase)
   *
   * @note Death is queued, not immediate, to maintain thread safety
   * @note Distance assertion ensures target is exactly 1 grid unit away
   * @see Peeps::queueForDeath() for deferred death processing
   * @see endOfSimulationStep() for death queue drainage
   */
  if (isEnabled(Action::KILL_FORWARD) && parameterMngrSingleton.killEnable) {
    constexpr float killThreshold = 0.5;  ///< Activation threshold [0.0, 1.0]; 0.5 is midlevel
    float level = actionLevels[Action::KILL_FORWARD];
    level = (std::tanh(level) + 1.0) / 2.0;  ///< Normalize to [0.0, 1.0]
    level *= responsivenessAdjusted;
    if (level > killThreshold && prob2bool((level - ACTION_MIN) / ACTION_RANGE)) {
      Coordinate otherLoc = indiv.loc + indiv.lastMoveDir;
      if (grid.isInBounds(otherLoc) && grid.isOccupiedAt(otherLoc)) {
        Individual& indiv2 = peeps.getIndiv(otherLoc);
        assert((indiv.loc - indiv2.loc).length() == 1);  ///< Verify adjacency
        peeps.queueForDeath(indiv2);
      }
    }
  }

  // ============================================================================
  // MOVEMENT ACTIONS: Vectorial Combination System
  // ============================================================================
  /**
   * Multiple movement neurons can activate simultaneously, and their effects
   * combine vectorially. This section accumulates all movement urges along
   * X and Y axes, then converts the vector sum into a discrete movement
   * offset through probabilistic rounding.
   *
   * **Movement types**:
   * - **Absolute**: MOVE_X, MOVE_Y, MOVE_EAST, MOVE_WEST, MOVE_NORTH, MOVE_SOUTH
   *   Direct contributions to X/Y accumulators
   *
   * - **Relative**: MOVE_FORWARD, MOVE_REVERSE, MOVE_LEFT, MOVE_RIGHT, MOVE_RL
   *   Based on lastMoveDir (agent's reference frame)
   *
   * - **Stochastic**: MOVE_RANDOM
   *   Random direction contribution for exploratory behavior
   *
   * **Algorithm**:
   * 1. Initialize accumulators (moveX, moveY) from MOVE_X/MOVE_Y or 0.0
   * 2. Accumulate all active movement neuron contributions
   * 3. Normalize via tanh() to bound to [-1, 1]
   * 4. Scale by adjusted responsiveness
   * 5. Convert to discrete offset via probabilistic rounding:
   *    - Probability of movement on axis = abs(moveX) or abs(moveY)
   *    - Direction on axis = sign(moveX) or sign(moveY)
   * 6. Validate target location (in-bounds, unoccupied)
   * 7. Queue movement if valid, otherwise discard
   *
   * **Example scenario**:
   * @code
   * // Multiple neurons active:
   * MOVE_FORWARD = 2.0, MOVE_LEFT = 1.0, MOVE_RANDOM = -0.5
   *
   * // Vector accumulation:
   * moveX = 2.0*forward.x + 1.0*left.x + -0.5*random.x
   * moveY = 2.0*forward.y + 1.0*left.y + -0.5*random.y
   *
   * // After tanh: bounded to [-1, 1]
   * // After responsiveness: scaled by behavioral sensitivity
   * // After prob2bool: discrete offset in {-1, 0, +1} per axis
   * @endcode
   *
   * **Thread safety**: Movement is queued, not applied, to avoid race conditions
   * on grid occupancy during parallel execution.
   */

  float level;                                                        ///< Temp variable for action level processing
  Coordinate offset;                                                  ///< Temp variable for directional calculations
  Coordinate lastMoveOffset = indiv.lastMoveDir.asNormalizedCoord();  ///< Reference frame for relative movement

  /**
   * Initialize movement accumulators with direct X/Y action values if enabled,
   * otherwise default to 0.0. These accumulators will hold the vector sum
   * of all movement urges (arbitrary float range before normalization).
   */
  float moveX = isEnabled(Action::MOVE_X) ? actionLevels[Action::MOVE_X] : 0.0;
  float moveY = isEnabled(Action::MOVE_Y) ? actionLevels[Action::MOVE_Y] : 0.0;

  /**
   * Accumulate cardinal direction movement urges.
   * Each enabled action adds (positive direction) or subtracts (negative direction)
   * from the appropriate axis accumulator.
   */
  if (isEnabled(Action::MOVE_EAST))
    moveX += actionLevels[Action::MOVE_EAST];  ///< Urge to move right (+X)
  if (isEnabled(Action::MOVE_WEST))
    moveX -= actionLevels[Action::MOVE_WEST];  ///< Urge to move left (-X)
  if (isEnabled(Action::MOVE_NORTH))
    moveY += actionLevels[Action::MOVE_NORTH];  ///< Urge to move up (+Y)
  if (isEnabled(Action::MOVE_SOUTH))
    moveY -= actionLevels[Action::MOVE_SOUTH];  ///< Urge to move down (-Y)

  /**
   * Accumulate ego-centric movement urges based on agent's reference frame.
   * These use lastMoveDir to define forward, and rotate for left/right.
   */
  if (isEnabled(Action::MOVE_FORWARD)) {
    level = actionLevels[Action::MOVE_FORWARD];
    moveX += lastMoveOffset.x * level;  ///< Scale forward direction by action level
    moveY += lastMoveOffset.y * level;
  }
  if (isEnabled(Action::MOVE_REVERSE)) {
    level = actionLevels[Action::MOVE_REVERSE];
    moveX -= lastMoveOffset.x * level;  ///< Reverse is negative forward
    moveY -= lastMoveOffset.y * level;
  }
  if (isEnabled(Action::MOVE_LEFT)) {
    level = actionLevels[Action::MOVE_LEFT];
    offset = indiv.lastMoveDir.rotate90DegCCW().asNormalizedCoord();  ///< Left is 90° CCW from forward
    moveX += offset.x * level;
    moveY += offset.y * level;
  }
  if (isEnabled(Action::MOVE_RIGHT)) {
    level = actionLevels[Action::MOVE_RIGHT];
    offset = indiv.lastMoveDir.rotate90DegCW().asNormalizedCoord();  ///< Right is 90° CW from forward
    moveX += offset.x * level;
    moveY += offset.y * level;
  }
  if (isEnabled(Action::MOVE_RL)) {
    level = actionLevels[Action::MOVE_RL];
    offset = indiv.lastMoveDir.rotate90DegCW().asNormalizedCoord();  ///< MOVE_RL is synonym for MOVE_RIGHT
    moveX += offset.x * level;
    moveY += offset.y * level;
  }

  /**
   * Accumulate stochastic movement urge.
   * Contributes a random direction scaled by action level, enabling
   * exploratory behavior when other movement neurons are uncertain.
   */
  if (isEnabled(Action::MOVE_RANDOM)) {
    level = actionLevels[Action::MOVE_RANDOM];
    offset = Dir::random8().asNormalizedCoord();  ///< Random direction in 8 compass directions
    moveX += offset.x * level;
    moveY += offset.y * level;
  }

  /**
   * Normalize and scale the accumulated movement vector.
   * - tanh() bounds values to [-1.0, 1.0] to prevent extreme velocities
   * - Responsiveness scaling dampens movement based on behavioral sensitivity
   */
  moveX = std::tanh(moveX);
  moveY = std::tanh(moveY);
  moveX *= responsivenessAdjusted;
  moveY *= responsivenessAdjusted;

  /**
   * Convert continuous movement vector to discrete offset via probabilistic rounding.
   * The probability of movement on each axis is the absolute value of the component.
   * The direction of movement is determined by the sign of the component.
   *
   * Example:
   * - moveX = -0.8 → 80% chance of moving, direction = -1 (West)
   * - moveY = 0.3  → 30% chance of moving, direction = +1 (North)
   * - Possible outcome: offset = (-1, 0) or (-1, +1) or (0, 0) or (0, +1)
   */
  int16_t probX = (int16_t)prob2bool(std::abs(moveX));  ///< Bernoulli trial: 0 or 1
  int16_t probY = (int16_t)prob2bool(std::abs(moveY));  ///< Bernoulli trial: 0 or 1

  /**
   * Extract direction from sign of movement components.
   * signum = -1 for negative values, +1 for non-negative values.
   */
  int16_t signumX = moveX < 0.0 ? -1 : 1;
  int16_t signumY = moveY < 0.0 ? -1 : 1;

  /**
   * Construct final movement offset.
   * Each component is in {-1, 0, +1}, representing one grid step in that direction.
   */
  Coordinate movementOffset((int16_t)(probX * signumX), (int16_t)(probY * signumY));

  /**
   * Validate and queue movement.
   * Movement succeeds only if target location is both in-bounds and unoccupied.
   * Invalid moves (blocked, out-of-bounds) are silently discarded with no penalty.
   */
  Coordinate newLoc = indiv.loc + movementOffset;
  if (grid.isInBounds(newLoc) && grid.isEmptyAt(newLoc)) {
    peeps.queueForMove(indiv, newLoc);
  }
}

}  // namespace BioSim
