/**
 * @file analysis.h
 * @brief Analysis and reporting functions for BioSim4
 *
 * Provides utilities for:
 * - Sensor/action name conversion for logging
 * - Diversity metrics calculation
 * - Per-generation reporting
 * - Individual neural net analysis
 */

#ifndef BIOSIM4_SRC_UTILS_ANALYSIS_H_
#define BIOSIM4_SRC_UTILS_ANALYSIS_H_

#include "../core/genetics/sensors-actions.h"

#include <string>

namespace BioSim {
inline namespace v1 {
namespace Utils {

/**
 * @brief Get human-readable name for a sensor
 * @param sensor Sensor enum value
 * @return String name (e.g., "LOC_X", "POPULATION")
 */
std::string sensorName(Core::Genetics::Sensor sensor);

/**
 * @brief Get human-readable name for an action
 * @param action Action enum value
 * @return String name (e.g., "MOVE_X", "EMIT_SIGNAL0")
 */
std::string actionName(Core::Genetics::Action action);

/**
 * @brief Print all enabled sensors and actions to stdout
 *
 * Lists active sensors (before NUM_SENSES) and actions (before NUM_ACTIONS).
 */
void printSensorsActions();

/**
 * @brief Calculate genetic diversity of surviving individuals
 *
 * Computes average genetic distance (Hamming distance) between all pairs
 * of survivors. Returns 0.0 for populations with 0-1 survivors.
 *
 * @return Diversity metric in range [0.0, 1.0]
 */
float geneticDiversity();

}  // namespace Utils

}  // namespace v1
}  // namespace BioSim

#endif  ///< BIOSIM4_SRC_UTILS_ANALYSIS_H_
