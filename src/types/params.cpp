/**
 * @file params.cpp
 * @brief Minimal parameter file (legacy ParamManager removed)
 *
 * ParamManager has been replaced by ConfigManager (configManager.h/cpp).
 * This file is kept for build compatibility but contains no implementation.
 *
 * @see configManager.h for the modern configuration system
 * @see params.h for the 5-step process to add new parameters
 */

#include "params.h"

namespace BioSim {
inline namespace v1 {
namespace Types {

// This file intentionally left minimal
// All parameter management is now handled by ConfigManager

}  // namespace Types
}  // namespace v1
}  // namespace BioSim
