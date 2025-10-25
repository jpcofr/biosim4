#ifndef BIOSIM4_INCLUDE_SIMULATOR_H_
#define BIOSIM4_INCLUDE_SIMULATOR_H_

/**
 * @file simulator.h
 * @brief Main simulator header and survival challenge definitions
 *
 * Provides the simulator entry point, global simulation objects, and
 * challenge type constants. Also see simulator.cpp for implementation.
 */

#include "basicTypes.h"  ///< types Dir, Coordinate, Polar
#include "grid.h"        ///< 2D world where the individuals live
#include "indiv.h"       ///< Individual agent data structure
#include "params.h"      ///< Configuration parameters
#include "peeps.h"       ///< Population container
#include "random.h"      ///< Random number generation
#include "signals.h"     ///< Pheromone layers

namespace BioSim {

/**
 * @defgroup Challenges Survival Challenges
 * @brief Available survival selection criteria
 *
 * Each challenge defines different selection pressure. Fine-tune challenges
 * by modifying the corresponding code in survival-criteria.cpp.
 * @{
 */
constexpr unsigned CHALLENGE_CIRCLE = 0;               ///< Survive if inside circle
constexpr unsigned CHALLENGE_RIGHT_HALF = 1;           ///< Survive in right half of world
constexpr unsigned CHALLENGE_RIGHT_QUARTER = 2;        ///< Survive in right quarter
constexpr unsigned CHALLENGE_STRING = 3;               ///< Survive along vertical line
constexpr unsigned CHALLENGE_CENTER_WEIGHTED = 4;      ///< Survival weighted by center distance
constexpr unsigned CHALLENGE_CENTER_UNWEIGHTED = 40;   ///< Survive in center region
constexpr unsigned CHALLENGE_CORNER = 5;               ///< Survive in corner
constexpr unsigned CHALLENGE_CORNER_WEIGHTED = 6;      ///< Survival weighted by corner distance
constexpr unsigned CHALLENGE_MIGRATE_DISTANCE = 7;     ///< Survive by migration distance
constexpr unsigned CHALLENGE_CENTER_SPARSE = 8;        ///< Survive in sparse center region
constexpr unsigned CHALLENGE_LEFT_EIGHTH = 9;          ///< Survive in left eighth
constexpr unsigned CHALLENGE_RADIOACTIVE_WALLS = 10;   ///< Survive away from walls
constexpr unsigned CHALLENGE_AGAINST_ANY_WALL = 11;    ///< Survive against any wall
constexpr unsigned CHALLENGE_TOUCH_ANY_WALL = 12;      ///< Survive if touched any wall
constexpr unsigned CHALLENGE_EAST_WEST_EIGHTHS = 13;   ///< Survive in east or west eighths
constexpr unsigned CHALLENGE_NEAR_BARRIER = 14;        ///< Survive near barriers
constexpr unsigned CHALLENGE_PAIRS = 15;               ///< Survive in pairs
constexpr unsigned CHALLENGE_LOCATION_SEQUENCE = 16;   ///< Survive by location sequence
constexpr unsigned CHALLENGE_ALTRUISM = 17;            ///< Survive through altruistic behavior
constexpr unsigned CHALLENGE_ALTRUISM_SACRIFICE = 18;  ///< Survive through self-sacrifice
/** @} */

/**
 * @brief Global parameter manager
 *
 * Manages simulator parameters from config file and runtime updates.
 */
extern ParamManager paramManager;

/**
 * @brief Read-only access to simulator parameters
 *
 * Singleton reference to current configuration parameters.
 */
extern const Params& parameterMngrSingleton;

/**
 * @brief Global simulation grid
 *
 * 2D arena where all individuals live and move.
 */
extern Grid grid;

/**
 * @brief Global pheromone layers
 *
 * Multi-layer signal container for chemical communication.
 */
extern Signals pheromones;

/**
 * @brief Global population container
 *
 * Manages all Individual instances in the simulation.
 */
extern Peeps peeps;

/**
 * @brief Main simulator entry point
 * @param argc Command-line argument count
 * @param argv Command-line argument vector
 *
 * Initializes simulation from config file, runs evolution loop with three
 * nested levels:
 * - Outer: each generation
 * - Middle: each simulation step within generation
 * - Inner: each individual (parallelized with OpenMP)
 *
 * Handles video generation, statistics logging, and genome analysis.
 */
extern void simulator(int argc, char** argv);

/**
 * @brief Modern simulator entry point (new config system)
 * @param params Pre-configured simulation parameters
 *
 * Overload that accepts directly configured Params struct from ConfigManager.
 * This is the modern interface - the argc/argv version is legacy.
 */
extern void simulator(const Params& params);

/**
 * @brief Visit all coordinates within circular radius
 * @param loc Center coordinate
 * @param radius Search radius (grid units)
 * @param f Function to call for each coordinate
 *
 * Calls f(Coordinate) once for each in-bounds location within the specified
 * circular area. Only visits cells within grid boundaries.
 */
extern void visitNeighborhood(Coordinate loc, float radius, std::function<void(Coordinate)> f);

}  // namespace BioSim

#endif  ///< BIOSIM4_INCLUDE_SIMULATOR_H_
