#ifndef BIOSIM4_SRC_CORE_AGENTS_PEEPS_H_
#define BIOSIM4_SRC_CORE_AGENTS_PEEPS_H_

/**
 * @file peeps.h
 * @brief Population manager for Individual agents
 *
 * Provides the Peeps class which manages all Individual instances in the simulation,
 * tracking their locations in the Grid and coordinating lifecycle events (spawn,
 * move, death).
 */

#include "../../core/world/grid.h"
#include "../../types/basicTypes.h"
#include "../../types/params.h"
#include "indiv.h"

#include <cstdint>
#include <vector>

namespace BioSim {
inline namespace v1 {
namespace Core {
namespace Agents {

struct Individual;

}  // namespace Agents

namespace World {
extern class Grid grid;
}  // namespace World

namespace Agents {

/**
 * @class Peeps
 * @brief Container and manager for all Individual agents in the simulation
 *
 * Peeps maintains a vector of all Individual instances (both alive and dead) and
 * coordinates their interactions with the Grid. Key responsibilities:
 *
 * - **Spawning**: Create new Individuals at random or specific locations
 * - **Movement**: Move Individuals between grid cells (queued to avoid conflicts)
 * - **Death**: Mark Individuals for removal (queued for thread-safe processing)
 * - **Culling**: Remove dead Individuals and compact the container
 *
 * ## Index Management
 * Each Individual has a unique index in range 1..0xfffe stored in the Grid at
 * the Individual's location. Grid cell value n refers to individuals[n].
 * **Index 0 is reserved** and individuals[0] is invalid.
 *
 * ## Queue Pattern
 * Death and movement operations are queued during simulation steps and drained
 * at safe synchronization points to avoid race conditions in parallel execution.
 *
 * ## Scope
 * Peeps manages only Individual location and aliveness. It does not modify
 * other Individual properties (genome, neural net, sensors, etc.).
 */
class Peeps {
 public:
  /**
   * @brief Construct empty Peeps container
   *
   * Creates container with zero individuals. Call initialize() before use.
   */
  Peeps();

  /**
   * @brief Initialize population with specified size
   * @param population Number of Individuals to create
   *
   * Spawns population count of Individuals at random empty grid locations
   * with random genomes.
   */
  void initialize(unsigned population);

  /**
   * @brief Queue an Individual for death
   * @param indiv Individual to mark for death
   *
   * Adds Individual to death queue. Actual removal happens in drainDeathQueue().
   * Thread-safe for concurrent queuing.
   */
  void queueForDeath(const Individual& indiv);

  /**
   * @brief Process all queued deaths
   *
   * Marks all queued Individuals as dead and clears their Grid locations.
   * Should be called at simulation step boundaries.
   */
  void drainDeathQueue();

  /**
   * @brief Queue an Individual to move to new location
   * @param indiv Individual to move
   * @param newLoc Destination coordinate
   *
   * Adds movement to queue. Actual move happens in drainMoveQueue().
   * Thread-safe for concurrent queuing.
   */
  void queueForMove(const Individual& indiv, Coordinate newLoc);

  /**
   * @brief Process all queued movements
   *
   * Executes all pending movements, updating Grid and Individual locations.
   * Should be called at simulation step boundaries.
   */
  void drainMoveQueue();

  /**
   * @brief Get current death queue size
   * @return Number of Individuals queued for death
   */
  unsigned deathQueueSize() const { return deathQueue.size(); }

  /**
   * @brief Get Individual at grid location (non-const)
   * @param loc Grid coordinate
   * @return Reference to Individual
   * @warning No bounds checking - ensure loc is occupied before calling
   */
  Individual& getIndiv(Coordinate loc) { return individuals[World::grid.at(loc)]; }

  /**
   * @brief Get Individual at grid location (const)
   * @param loc Grid coordinate
   * @return const reference to Individual
   * @warning No bounds checking - ensure loc is occupied before calling
   */
  const Individual& getIndiv(Coordinate loc) const { return individuals[World::grid.at(loc)]; }

  /**
   * @brief Direct access by index (non-const)
   * @param index Individual index (1..0xfffe)
   * @return Reference to Individual
   * @warning Index 0 is reserved and invalid
   */
  Individual& operator[](uint16_t index) { return individuals[index]; }

  /**
   * @brief Direct access by index (const)
   * @param index Individual index (1..0xfffe)
   * @return const reference to Individual
   * @warning Index 0 is reserved and invalid
   */
  Individual const& operator[](uint16_t index) const { return individuals[index]; }

 private:
  std::vector<Individual> individuals;                     ///< All Individuals (index 0 reserved)
  std::vector<uint16_t> deathQueue;                        ///< Indices of Individuals to kill
  std::vector<std::pair<uint16_t, Coordinate>> moveQueue;  ///< (index, destination) pairs
};

}  // namespace Agents
}  // namespace Core
}  // namespace v1
}  // namespace BioSim

// Backward compatibility aliases
namespace BioSim {
using Core::Agents::Peeps;
}  // namespace BioSim

#endif  ///< BIOSIM4_SRC_CORE_AGENTS_PEEPS_H_
