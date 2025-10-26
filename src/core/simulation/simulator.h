#ifndef BIOSIM4_SRC_CORE_SIMULATION_SIMULATOR_H_
#define BIOSIM4_SRC_CORE_SIMULATION_SIMULATOR_H_

/**
 * @file simulator.h
 * @brief Main simulator header and survival challenge definitions
 *
 * Provides the simulator entry point, global simulation objects, and
 * challenge type constants. Also see simulator.cpp for implementation.
 */

#include "../../types/basicTypes.h"  ///< types Dir, Coordinate, Polar
#include "../../types/params.h"      ///< Configuration parameters
#include "../../utils/random.h"      ///< Random number generation
#include "../agents/indiv.h"         ///< Individual agent data structure
#include "../agents/peeps.h"         ///< Population container
#include "../world/grid.h"           ///< 2D world where the individuals live
#include "../world/signals.h"        ///< Pheromone layers

#include <functional>

namespace BioSim {
inline namespace v1 {
namespace Core {
namespace Simulation {

constexpr unsigned CHALLENGE_CIRCLE = 0;
constexpr unsigned CHALLENGE_RIGHT_HALF = 1;
constexpr unsigned CHALLENGE_RIGHT_QUARTER = 2;
constexpr unsigned CHALLENGE_STRING = 3;
constexpr unsigned CHALLENGE_CENTER_WEIGHTED = 4;
constexpr unsigned CHALLENGE_CENTER_UNWEIGHTED = 40;
constexpr unsigned CHALLENGE_CORNER = 5;
constexpr unsigned CHALLENGE_CORNER_WEIGHTED = 6;
constexpr unsigned CHALLENGE_MIGRATE_DISTANCE = 7;
constexpr unsigned CHALLENGE_CENTER_SPARSE = 8;
constexpr unsigned CHALLENGE_LEFT_EIGHTH = 9;
constexpr unsigned CHALLENGE_RADIOACTIVE_WALLS = 10;
constexpr unsigned CHALLENGE_AGAINST_ANY_WALL = 11;
constexpr unsigned CHALLENGE_TOUCH_ANY_WALL = 12;
constexpr unsigned CHALLENGE_EAST_WEST_EIGHTHS = 13;
constexpr unsigned CHALLENGE_NEAR_BARRIER = 14;
constexpr unsigned CHALLENGE_PAIRS = 15;
constexpr unsigned CHALLENGE_LOCATION_SEQUENCE = 16;
constexpr unsigned CHALLENGE_ALTRUISM = 17;
constexpr unsigned CHALLENGE_ALTRUISM_SACRIFICE = 18;

extern const Types::Params& parameterMngrSingleton;

void simulator(const Types::Params& params);

// Test helper function to initialize global params
void initParamsForTesting(uint16_t gridSizeX = 128, uint16_t gridSizeY = 128);

using World::visitNeighborhood;

}  // namespace Simulation

// Global singletons (declared in their proper namespaces, defined in simulator.cpp)
namespace World {
extern Grid grid;
extern Signals pheromones;
}  // namespace World

namespace Agents {
extern Peeps peeps;
}  // namespace Agents

}  // namespace Core
}  // namespace v1

using Core::Agents::peeps;
using Core::Simulation::CHALLENGE_AGAINST_ANY_WALL;
using Core::Simulation::CHALLENGE_ALTRUISM;
using Core::Simulation::CHALLENGE_ALTRUISM_SACRIFICE;
using Core::Simulation::CHALLENGE_CENTER_SPARSE;
using Core::Simulation::CHALLENGE_CENTER_UNWEIGHTED;
using Core::Simulation::CHALLENGE_CENTER_WEIGHTED;
using Core::Simulation::CHALLENGE_CIRCLE;
using Core::Simulation::CHALLENGE_CORNER;
using Core::Simulation::CHALLENGE_CORNER_WEIGHTED;
using Core::Simulation::CHALLENGE_EAST_WEST_EIGHTHS;
using Core::Simulation::CHALLENGE_LEFT_EIGHTH;
using Core::Simulation::CHALLENGE_LOCATION_SEQUENCE;
using Core::Simulation::CHALLENGE_MIGRATE_DISTANCE;
using Core::Simulation::CHALLENGE_NEAR_BARRIER;
using Core::Simulation::CHALLENGE_PAIRS;
using Core::Simulation::CHALLENGE_RADIOACTIVE_WALLS;
using Core::Simulation::CHALLENGE_RIGHT_HALF;
using Core::Simulation::CHALLENGE_RIGHT_QUARTER;
using Core::Simulation::CHALLENGE_STRING;
using Core::Simulation::CHALLENGE_TOUCH_ANY_WALL;
using Core::Simulation::initParamsForTesting;
using Core::Simulation::parameterMngrSingleton;
using Core::Simulation::simulator;
using Core::World::grid;
using Core::World::pheromones;
using Core::World::visitNeighborhood;

}  // namespace BioSim

#endif  // BIOSIM4_SRC_CORE_SIMULATION_SIMULATOR_H_
