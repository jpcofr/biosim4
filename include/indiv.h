#ifndef BIOSIM4_INCLUDE_INDIV_H_
#define BIOSIM4_INCLUDE_INDIV_H_

/**
 * @file indiv.h
 * @brief Individual agent representation and neural network interface
 *
 * Defines the Individual structure that represents one agent in the simulation.
 * Each individual has a genome, a neural network brain derived from that genome,
 * sensory capabilities, and action outputs.
 *
 * @see Peeps for container class managing all individuals
 */

#include "basicTypes.h"
#include "genome-neurons.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace BioSim {

/**
 * @struct Individual
 * @brief Represents a single autonomous agent in the simulation
 *
 * An Individual is a creature with:
 * - A genome that defines its neural network structure
 * - A neural network brain that processes sensory inputs and produces action outputs
 * - Physical properties (location, age, responsiveness)
 * - Behavioral state (movement direction, challenge progress)
 *
 * The individual's behavior emerges from its neural network, which is created
 * from its genome via createWiringFromGenome().
 */
struct Individual {
  bool alive;           ///< Whether the individual is currently alive
  uint16_t index;       ///< Index into peeps[] container
  Coordinate loc;       ///< Current location in grid[][]
  Coordinate birthLoc;  ///< Location where individual was born
  unsigned age;         ///< Simulation steps since birth

  Genome genome;           ///< Genetic code defining neural network structure
  NeuralNet nnet;          ///< Neural network derived from genome
  float responsiveness;    ///< Behavioral responsiveness (0.0..1.0, 0 = inactive)
  unsigned oscPeriod;      ///< Oscillation period (2..4*p.stepsPerGeneration, TBD)
  unsigned longProbeDist;  ///< Distance for long-range forward obstruction probes
  Dir lastMoveDir;         ///< Direction of last movement action
  unsigned challengeBits;  ///< Bitfield tracking challenge accomplishments

  /**
   * @brief Execute one neural network forward pass
   * @param simStep Current simulation step number
   * @return Array of action output values (0.0..1.0)
   *
   * Reads all sensors via getSensor(), processes through neural network,
   * and returns action outputs. Does not execute actions - that's done
   * by executeActions().
   */
  std::array<float, Action::NUM_ACTIONS> feedForward(unsigned simStep);

  /**
   * @brief Read a single sensor value
   * @param sensor Sensor type to read
   * @param simStep Current simulation step number
   * @return Sensor value in range [SENSOR_MIN, SENSOR_MAX]
   */
  float getSensor(Sensor sensor, unsigned simStep) const;

  /**
   * @brief Initialize a new individual
   * @param index Index in peeps[] container
   * @param loc Starting location in grid
   * @param genome Genome (moved into individual)
   */
  void initialize(uint16_t index, Coordinate loc, Genome&& genome);

  /**
   * @brief Create neural network from genome
   *
   * Converts genome representation into executable neural network structure.
   * Validates connections and remaps neuron indices. Called after genome
   * initialization or mutation.
   */
  void createWiringFromGenome();

  /**
   * @brief Print neural network structure to stdout
   *
   * Displays connections, weights, and neuron types in human-readable format.
   */
  void printNeuralNet() const;

  /**
   * @brief Print neural network as igraph edge list
   *
   * Outputs format suitable for graph-nnet.py visualization tool.
   */
  void printIGraphEdgeList() const;

  /**
   * @brief Print genome in hex and mnemonic format
   *
   * Displays genome as both raw hex values and human-readable gene names.
   */
  void printGenome() const;
};

}  // namespace BioSim

#endif  ///< BIOSIM4_INCLUDE_INDIV_H_
