#ifndef BIOSIM4_SRC_CORE_GENETICS_GENOME_NEURONS_H_
#define BIOSIM4_SRC_CORE_GENETICS_GENOME_NEURONS_H_

/**
 * @file genome-neurons.h
 * @brief Genome and neural network definitions
 *
 * Defines the genetic encoding system and neural network structures:
 * - **Gene**: Single synaptic connection specification
 * - **Genome**: Collection of genes defining entire neural network
 * - **NeuralNet**: Executable neural network derived from genome
 *
 * ## Architecture
 * - Each gene specifies one synaptic connection (source → sink with weight)
 * - Sources: sensors or neurons
 * - Sinks: actions or neurons
 * - Topology: Free-form (forward, backward, sideways connections allowed)
 * - No explicit layers - connections form arbitrary directed graphs
 *
 * ## Neuron Indexing
 * - Genome uses 15-bit unsigned indices for neurons
 * - Indices are remapped via modulo to range 0..p.genomeMaxLength-1
 * - Connected neurons get sequential indices starting at 0 in NeuralNet
 */

#include "../../utils/random.h"
#include "sensors-actions.h"

#include <cmath>
#include <cstdint>
#include <vector>

namespace BioSim {
inline namespace v1 {
namespace Core {
namespace Genetics {

/// Type identifier for sensor (always a source)
constexpr uint8_t SENSOR = 1;

/// Type identifier for action (always a sink)
constexpr uint8_t ACTION = 1;

/// Type identifier for neuron (can be source or sink)
constexpr uint8_t NEURON = 0;

/**
 * @struct Gene
 * @brief Encodes a single synaptic connection in a neural network
 *
 * Each gene specifies:
 * - Input source: sensor or neuron (identified by type + index)
 * - Output sink: action or neuron (identified by type + index)
 * - Connection weight: signed 16-bit value scaled to floating point
 *
 * The weight is scaled to provide fine resolution near zero.
 */
struct Gene {
  uint16_t sourceType : 1;  ///< SENSOR or NEURON
  uint16_t sourceNum : 7;   ///< Source index (0..127)
  uint16_t sinkType : 1;    ///< NEURON or ACTION
  uint16_t sinkNum : 7;     ///< Sink index (0..127)
  int16_t weight;           ///< Connection weight (-32768..32767)

  /// Weight scaling constant
  static constexpr float f1 = 8.0;

  /// Weight range constant
  static constexpr float f2 = 64.0;

  /**
   * @brief Convert integer weight to floating point
   * @return Weight as float in approximate range [-4, 4]
   */
  float weightAsFloat() const { return weight / 8192.0; }

  /**
   * @brief Generate random weight value
   * @return Random int16_t in range [-32768, 32767]
   */
  static int16_t makeRandomWeight() { return randomUint(0, 0xffff) - 0x8000; }
};

/**
 * @typedef Genome
 * @brief Collection of genes defining an individual's neural network
 *
 * A genome is a variable-length vector of genes. Each gene translates to
 * one connection in the individual's neural network. The genome is the
 * heritable genetic code that determines behavior.
 */
typedef std::vector<Gene> Genome;

/**
 * @struct NeuralNet
 * @brief Executable neural network derived from a genome
 *
 * Represents an individual's "brain" - a directed graph of weighted connections
 * between sensors, neurons, and actions. The topology is free-form with no
 * layer restrictions.
 *
 * ## Activation
 * - Currently uses hardcoded activation function (likely std::tanh())
 * - Future: may be specified by genome
 *
 * ## Signal Flow
 * - Sensor inputs: raw sensor values (float, sensor-dependent)
 * - Neuron outputs: calculated via activation function
 * - Action outputs: interpreted by action nodes to trigger behaviors
 *
 * ## Neuron States
 * - **Driven neurons**: receive input connections, compute output
 * - **Undriven neurons**: no inputs, maintain fixed output value
 */
struct NeuralNet {
  std::vector<Gene> connections;  ///< Active connections (subset of genome genes)

  /**
   * @struct Neuron
   * @brief Single neuron state in the neural network
   */
  struct Neuron {
    float output;  ///< Current output value
    bool driven;   ///< true if neuron has input connections
  };

  std::vector<Neuron> neurons;  ///< All neurons in the network
};

/**
 * @brief Initial output value for newly created neurons
 * @return 0.5 (midpoint of [0, 1] range)
 *
 * When a new population is generated, all neuron outputs are initialized
 * to this value to provide consistent starting conditions.
 */
constexpr float initialNeuronOutput() {
  return 0.5;
}

/**
 * @brief Generate a random gene with random connection and weight
 * @return Randomly initialized Gene
 */
extern Gene makeRandomGene();

/**
 * @brief Generate a random genome of configured length
 * @return Randomly initialized Genome
 */
extern Genome makeRandomGenome();

/**
 * @brief Unit test for genome-to-neural-net conversion
 *
 * Tests createWiringFromGenome() functionality including edge cases.
 */
extern void unitTestConnectNeuralNetWiringFromGenome();

/**
 * @brief Calculate similarity between two genomes
 * @param g1 First genome
 * @param g2 Second genome
 * @return Similarity score in range [0.0, 1.0] where 1.0 is identical
 *
 * Uses configured comparison method (Jaro-Winkler or Hamming distance).
 */
extern float genomeSimilarity(const Genome& g1, const Genome& g2);

/**
 * @brief Calculate genetic diversity across entire population
 * @return Diversity score in range [0.0, 1.0] where 1.0 is maximum diversity
 *
 * Measures average pairwise genome dissimilarity in the population.
 */
extern float geneticDiversity();

}  // namespace Genetics
}  // namespace Core
}  // namespace v1
}  // namespace BioSim

// Backward compatibility aliases
namespace BioSim {
using Core::Genetics::ACTION;
using Core::Genetics::Gene;
using Core::Genetics::geneticDiversity;
using Core::Genetics::Genome;
using Core::Genetics::genomeSimilarity;
using Core::Genetics::initialNeuronOutput;
using Core::Genetics::makeRandomGene;
using Core::Genetics::makeRandomGenome;
using Core::Genetics::NeuralNet;
using Core::Genetics::NEURON;
using Core::Genetics::SENSOR;
using Core::Genetics::unitTestConnectNeuralNetWiringFromGenome;
}  // namespace BioSim

#endif  ///< BIOSIM4_SRC_CORE_GENETICS_GENOME_NEURONS_H_
