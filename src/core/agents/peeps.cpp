/**
 * @file peeps.cpp
 * @brief Implementation of the Peeps population manager
 *
 * This file implements the Peeps class which manages the lifecycle and spatial
 * organization of all Individual agents in the simulation. It coordinates with
 * the global Grid object to track agent locations and provides thread-safe
 * queueing mechanisms for movement and death operations.
 *
 * ## Threading Model
 * The simulation uses OpenMP parallelization, so movement and death operations
 * are queued during parallel execution and drained at synchronization points
 * to avoid race conditions.
 *
 * ## Index Space
 * Individual indices range from 1 to population (index 0 is reserved as an
 * invalid/empty marker). The Grid stores these indices at each cell location.
 *
 * @see peeps.h for class documentation
 * @see Individual for agent representation
 * @see Grid for spatial organization
 */

#include "../../core/simulation/simulator.h"

#include <cassert>
#include <iostream>
#include <numeric>
#include <utility>

namespace BioSim {
inline namespace v1 {
namespace Core {
namespace Agents {

/**
 * @brief Default constructor for Peeps container
 *
 * Constructs an empty Peeps object. Call initialize() before use to allocate
 * space for the population.
 */
Peeps::Peeps() {}

/**
 * @brief Initialize the population container with specified capacity
 *
 * Allocates space in the individuals vector for the entire population plus
 * one reserved slot at index 0. This must be called after construction and
 * before spawning any individuals.
 *
 * @param population Maximum number of individuals that can exist simultaneously
 *
 * @note Index 0 is reserved as an invalid/empty marker used by the Grid to
 *       indicate empty cells. Valid individual indices are 1..population.
 *
 * @post individuals.size() == population + 1
 * @post individuals[0] is unused (reserved)
 */
void Peeps::initialize(unsigned population) {
  /// Index 0 is reserved, so add one:
  individuals.resize(population + 1);
}

/**
 * @brief Queue an individual for removal from the simulation
 *
 * Adds the individual's index to the death queue for processing at the end
 * of the current simulation step. This function is thread-safe and can be
 * called during OpenMP parallel regions.
 *
 * The individual will remain alive and visible in the Grid until
 * drainDeathQueue() is called at the end of the simulation step. This deferred
 * removal pattern prevents race conditions when multiple threads might
 * interact with the same individual.
 *
 * @param individual The individual to mark for death (must be alive)
 *
 * @pre individual.alive == true
 * @post individual.index is added to deathQueue
 *
 * @note Thread-safe: Uses OpenMP critical section
 * @note It's safe to queue the same individual multiple times; duplicates
 *       are handled correctly by drainDeathQueue()
 * @note Do not call for already-dead individuals (assertion will fail)
 *
 * @see drainDeathQueue() for queue processing
 */
void Peeps::queueForDeath(const Individual& individual) {
  assert(individual.alive);

#pragma omp critical
  {
    deathQueue.push_back(individual.index);
  }
}

/**
 * @brief Process all queued deaths at the end of a simulation step
 *
 * Executes all pending death operations accumulated during the simulation
 * step. For each queued individual:
 * 1. Removes its index from the Grid cell at its location (sets cell to 0)
 * 2. Marks the individual as dead (alive = false)
 *
 * This must be called in single-threaded mode at a synchronization point
 * (typically at the end of each simulation step, after all parallel processing
 * has completed).
 *
 * @pre Must be called from single-threaded context (not during OpenMP parallel)
 * @post All individuals in deathQueue are marked dead and removed from Grid
 * @post deathQueue is empty
 *
 * @note Duplicates in the queue are harmless; setting alive=false multiple
 *       times has no adverse effect
 *
 * @see queueForDeath() for adding individuals to the queue
 * @see endOfSimulationStep() in simulator.cpp for typical call site
 */
void Peeps::drainDeathQueue() {
  for (uint16_t index : deathQueue) {
    auto& indiv = peeps[index];
    World::grid.set(indiv.loc, 0);
    indiv.alive = false;
  }
  deathQueue.clear();
}

/**
 * @brief Queue an individual to move to a new location
 *
 * Adds a movement request to the move queue for processing at the end of the
 * current simulation step. This function is thread-safe and can be called
 * during OpenMP parallel regions.
 *
 * The individual will not actually move until drainMoveQueue() is called at
 * the end of the simulation step. This deferred execution pattern prevents
 * race conditions when multiple threads might move individuals to overlapping
 * locations.
 *
 * @param indiv The individual to move (must be alive)
 * @param newLoc The target grid location
 *
 * @pre indiv.alive == true
 * @post A movement record (indiv.index, newLoc) is added to moveQueue
 *
 * @note Thread-safe: Uses OpenMP critical section
 * @note If multiple individuals are queued to move to the same location,
 *       only the first one processed will succeed; others will remain at
 *       their original locations
 * @note Should only be called for living individuals (assertion will fail
 *       for dead individuals)
 * @note Movement distance is typically one 8-neighbor cell, but arbitrary
 *       distances are supported by drainMoveQueue()
 *
 * @see drainMoveQueue() for queue processing
 * @see executeActions() in executeActions.cpp for typical usage
 */
void Peeps::queueForMove(const Individual& indiv, Coordinate newLoc) {
  assert(indiv.alive);

#pragma omp critical
  {
    auto record = std::make_pair<uint16_t, Coordinate>(uint16_t(indiv.index), Coordinate(newLoc));
    moveQueue.push_back(record);
  }
}

/**
 * @brief Process all queued movements at the end of a simulation step
 *
 * Executes all pending movement operations accumulated during the simulation
 * step. For each queued movement:
 * 1. Checks if the individual is still alive (may have been killed since queueing)
 * 2. Checks if the target location is empty in the Grid
 * 3. If both conditions are met:
 *    - Clears the individual's old Grid cell (sets to 0)
 *    - Updates the new Grid cell with the individual's index
 *    - Updates the individual's location and lastMoveDir
 *
 * This must be called in single-threaded mode at a synchronization point
 * (typically at the end of each simulation step, after death queue processing).
 *
 * ## Movement Distance
 * Movements are typically one 8-neighbor cell distance (N, NE, E, SE, S, SW,
 * W, NW), but arbitrary distances are supported.
 *
 * ## Collision Handling
 * If the target location is occupied, the movement is silently ignored and
 * the individual remains at its current location. When multiple individuals
 * are queued to move to the same location, only the first one processed
 * succeeds.
 *
 * @pre Must be called from single-threaded context (not during OpenMP parallel)
 * @pre drainDeathQueue() should be called before this to handle dead individuals
 * @post All valid movements are executed, updating both Grid and Individual.loc
 * @post moveQueue is empty
 *
 * @note Dead individuals are skipped (they may have been killed after queueing)
 * @note Failed movements (target occupied) are silent; no error or retry
 *
 * @see queueForMove() for adding movements to the queue
 * @see endOfSimulationStep() in simulator.cpp for typical call site
 */
void Peeps::drainMoveQueue() {
  for (auto& moveRecord : moveQueue) {
    auto& indiv = peeps[moveRecord.first];
    if (indiv.alive) {
      Coordinate newLoc = moveRecord.second;
      Dir moveDir = (newLoc - indiv.loc).asDir();
      if (World::grid.isEmptyAt(newLoc)) {
        World::grid.set(indiv.loc, 0);
        World::grid.set(newLoc, indiv.index);
        indiv.loc = newLoc;
        indiv.lastMoveDir = moveDir;
      }
    }
  }
  moveQueue.clear();
}

}  // namespace Agents
}  // namespace Core
}  // namespace v1
}  // namespace BioSim
