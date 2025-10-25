#ifndef BIOSIM4_INCLUDE_SENSORS_ACTIONS_H_
#define BIOSIM4_INCLUDE_SENSORS_ACTIONS_H_

/**
 * @file sensors-actions.h
 * @brief Sensor inputs and action outputs for neural networks
 *
 * Defines which sensor input neurons and which action output neurons are
 * compiled into the simulator. This file can be modified to create a simulator
 * executable that supports only a subset of all possible sensors or actions.
 *
 * ## CRITICAL: Enable/Disable Pattern
 * **FRAGILE DESIGN - Handle with care!**
 *
 * ### How to Enable/Disable Sensors
 * - Place desired sensors **BEFORE** `NUM_SENSES` marker in the Sensor enum
 * - Sensors **AFTER** `NUM_SENSES` are disabled (compiled out)
 * - Reordering changes functionality, not just compilation!
 *
 * ### How to Enable/Disable Actions
 * - Place desired actions **BEFORE** `NUM_ACTIONS` marker in the Action enum
 * - Actions **AFTER** `NUM_ACTIONS` are disabled (compiled out)
 * - Reordering changes functionality, not just compilation!
 *
 * ### When Adding New Items
 * Also update the name functions in analysis.cpp.
 *
 * ## Value Ranges
 * These ranges are now hardcoded and assumed throughout the codebase:
 * - Sensors: [0.0, 1.0]
 * - Neurons: [-1.0, 1.0]
 * - Actions: [0.0, 1.0]
 */

#include <string>

namespace BioSim {

/// Minimum sensor output value
constexpr float SENSOR_MIN = 0.0;

/// Maximum sensor output value
constexpr float SENSOR_MAX = 1.0;

/// Sensor value range
constexpr float SENSOR_RANGE = SENSOR_MAX - SENSOR_MIN;

/// Minimum neuron output value
constexpr float NEURON_MIN = -1.0;

/// Maximum neuron output value
constexpr float NEURON_MAX = 1.0;

/// Neuron value range
constexpr float NEURON_RANGE = NEURON_MAX - NEURON_MIN;

/// Minimum action input value
constexpr float ACTION_MIN = 0.0;

/// Maximum action input value
constexpr float ACTION_MAX = 1.0;

/// Action value range
constexpr float ACTION_RANGE = ACTION_MAX - ACTION_MIN;

/**
 * @enum Sensor
 * @brief Input sensors available to neural networks
 *
 * Each sensor provides information about the individual (I) or world (W):
 * - **I**: Data about the individual (stored in Individual struct)
 * - **W**: Data about environment (stored in Peeps or Grid)
 *
 * **Only sensors before NUM_SENSES are enabled!**
 */
enum Sensor {
  LOC_X,              ///< I: Distance from left edge
  LOC_Y,              ///< I: Distance from bottom edge
  BOUNDARY_DIST_X,    ///< I: X distance to nearest world edge
  BOUNDARY_DIST,      ///< I: Euclidean distance to nearest edge
  BOUNDARY_DIST_Y,    ///< I: Y distance to nearest world edge
  GENETIC_SIM_FWD,    ///< I: Genetic similarity of individual directly ahead
  LAST_MOVE_DIR_X,    ///< I: X component of last movement
  LAST_MOVE_DIR_Y,    ///< I: Y component of last movement
  LONGPROBE_POP_FWD,  ///< W: Population detected by long-range forward probe
  LONGPROBE_BAR_FWD,  ///< W: Barrier detected by long-range forward probe
  POPULATION,         ///< W: Population density in local neighborhood
  POPULATION_FWD,     ///< W: Population density along forward-reverse axis
  POPULATION_LR,      ///< W: Population density along left-right axis
  OSC1,               ///< I: Internal oscillator value (sine wave)
  AGE,                ///< I: Individual's age in simulation steps
  BARRIER_FWD,        ///< W: Barrier distance along forward-reverse axis
  BARRIER_LR,         ///< W: Barrier distance along left-right axis
  RANDOM,             ///< Random value (uniform distribution 0.0..1.0)
  SIGNAL0,            ///< W: Pheromone layer 0 strength in neighborhood
  SIGNAL0_FWD,        ///< W: Pheromone layer 0 along forward-reverse axis
  SIGNAL0_LR,         ///< W: Pheromone layer 0 along left-right axis
  NUM_SENSES,         ///< **MARKER: Sensors after this are DISABLED**
};

/**
 * @enum Action
 * @brief Output actions available to neural networks
 *
 * Each action affects the individual (I) or world (W):
 * - **I**: Internal effect (Individual properties)
 * - **W**: External effect (movement, signals, affecting Peeps/Grid)
 *
 * **Only actions before NUM_ACTIONS are enabled!**
 */
enum Action {
  MOVE_X,                 ///< W: Move along X axis (positive = right)
  MOVE_Y,                 ///< W: Move along Y axis (positive = up)
  MOVE_FORWARD,           ///< W: Continue in last movement direction
  MOVE_RL,                ///< W: Move along left-right axis
  MOVE_RANDOM,            ///< W: Move in random direction
  SET_OSCILLATOR_PERIOD,  ///< I: Adjust internal oscillator frequency
  SET_LONGPROBE_DIST,     ///< I: Adjust long-range probe distance
  SET_RESPONSIVENESS,     ///< I: Adjust behavioral responsiveness (0=inactive)
  EMIT_SIGNAL0,           ///< W: Emit pheromone on layer 0
  MOVE_EAST,              ///< W: Move east (positive X)
  MOVE_WEST,              ///< W: Move west (negative X)
  MOVE_NORTH,             ///< W: Move north (positive Y)
  MOVE_SOUTH,             ///< W: Move south (negative Y)
  MOVE_LEFT,              ///< W: Move left relative to last direction
  MOVE_RIGHT,             ///< W: Move right relative to last direction
  MOVE_REVERSE,           ///< W: Move backward (opposite of last direction)
  NUM_ACTIONS,            ///< **MARKER: Actions after this are DISABLED**
  KILL_FORWARD,           ///< W: Kill individual directly ahead (DISABLED)
};

/**
 * @brief Get human-readable name for a sensor
 * @param sensor Sensor enum value
 * @return String name (e.g., "LOC_X", "POPULATION")
 */
extern std::string sensorName(Sensor sensor);

/**
 * @brief Get human-readable name for an action
 * @param action Action enum value
 * @return String name (e.g., "MOVE_X", "EMIT_SIGNAL0")
 */
extern std::string actionName(Action action);

/**
 * @brief Print all enabled sensors and actions to stdout
 *
 * Lists active sensors (before NUM_SENSES) and actions (before NUM_ACTIONS).
 */
extern void printSensorsActions();

}  // namespace BioSim

#endif  ///< BIOSIM4_INCLUDE_SENSORS_ACTIONS_H_
