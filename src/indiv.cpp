/**
 * @file indiv.cpp
 * @brief Implementation of Individual agent initialization
 *
 * Contains the initialization logic for spawning new individuals in the simulation.
 * Each individual is created with a genome that determines its neural network structure
 * and behavioral characteristics.
 */

#include "simulator.h"

#include <cassert>
#include <iostream>

namespace BioSim {

/**
 * @brief Initialize a newly spawned individual
 *
 * Sets up all member variables for a new agent entering the simulation.
 * The individual is placed at the specified location, assigned an index,
 * and its neural network is created from the provided genome.
 *
 * Initial behavioral parameters:
 * - Age starts at 0
 * - Oscillation period set to 34 steps (magic number, needs refactoring)
 * - Responsiveness at 0.5 (midrange, may be adjusted by action activation function)
 * - Random initial movement direction
 * - Challenge bits cleared (no accomplishments yet)
 *
 * @param index_ Unique identifier and index into peeps[] container
 * @param loc_ Starting grid coordinates for this individual
 * @param genome_ Genetic code defining neural network (moved, not copied)
 *
 * @note This function must be called before the individual can participate in simulation
 * @note The genome is moved (not copied) for efficiency
 * @note After initialization, createWiringFromGenome() is automatically called
 *
 * @see createWiringFromGenome() for neural network construction
 * @see Peeps::spawnNewGeneration() for typical usage context
 */
void Individual::initialize(uint16_t index_, Coordinate loc_, Genome&& genome_) {
  index = index_;
  loc = loc_;
  // birthLoc = loc_;  // Currently unused - may be needed for future features
  grid.set(loc_, index_);
  age = 0;
  oscPeriod = 34;  // TODO: Define as named constant (e.g., DEFAULT_OSC_PERIOD)
  alive = true;
  lastMoveDir = Dir::random8();
  responsiveness = 0.5;  // Midrange initial value (range 0.0..1.0)
  longProbeDist = parameterMngrSingleton.longProbeDistance;
  challengeBits = (unsigned)false;  // No challenges accomplished yet
  genome = std::move(genome_);
  createWiringFromGenome();
}

}  // namespace BioSim
