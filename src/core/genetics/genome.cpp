/**
 * @file genome.cpp
 * @brief Genome creation, mutation, and neural network wiring implementation
 *
 * This file implements the genetic encoding system that translates genomes into
 * functional neural networks. Key processes:
 *
 * ## Genome → Neural Network Pipeline
 * 1. **Renumbering**: Map 16-bit genome neuron indices → 0..maxNumberNeurons-1
 * 2. **Node Discovery**: Build list of all neurons referenced in connections
 * 3. **Culling**: Remove neurons with no outputs (or only self-connections)
 * 4. **Remapping**: Renumber remaining neurons sequentially from 0
 * 5. **Wiring**: Create final connection list (neurons first, then actions)
 *
 * ## Genetic Operations
 * - **Mutation**: Point mutations (bit flips), insertions, deletions
 * - **Reproduction**: Sexual (two-parent crossover) or asexual (single parent)
 * - **Selection**: Fitness-based or random parent selection
 *
 * ## Design Notes
 * - Neurons are culled if they don't contribute to action outputs
 * - Connection ordering optimizes feedForward execution (see feedForward.cpp)
 * - Genome length can vary if configured (insertions/deletions enabled)
 *
 * @see createWiringFromGenome() for the main genome→neural net conversion
 * @see generateChildGenome() for reproduction and mutation
 */

#include "../../core/simulation/simulator.h"
#include "../../utils/random.h"

#include <spdlog/fmt/fmt.h>

#include <cassert>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>

namespace BioSim {
inline namespace v1 {
namespace Core {
namespace Genetics {

/**
 * @struct Node
 * @brief Temporary structure for neural network construction from genome
 *
 * Used during genome→neural net conversion to track neuron usage and connectivity.
 * This helps identify and remove "useless" neurons (those with no outputs or only
 * self-connections) before final wiring.
 *
 * ## Culling Criteria
 * A neuron is removed if:
 * - `numOutputs == 0` (feeds nothing)
 * - `numOutputs == numSelfInputs` (only feeds itself)
 *
 * ## Renumbering Process
 * 1. Original genome uses 16-bit neuron indices
 * 2. First renumbering: modulo to 0..maxNumberNeurons-1
 * 3. After culling: `remappedNumber` provides sequential 0-based indices
 *
 * @see makeNodeList() for initial node discovery
 * @see cullUselessNeurons() for removal logic
 */
struct Node {
  uint16_t remappedNumber;                      ///< Final sequential neuron index (0-based)
  uint16_t numOutputs;                          ///< Total outgoing connections from this neuron
  uint16_t numSelfInputs;                       ///< Number of connections feeding back to self
  uint16_t numInputsFromSensorsOrOtherNeurons;  ///< Incoming connections (non-self)
};

/**
 * @typedef NodeMap
 * @brief Map of neuron index to Node metadata during network construction
 *
 * Key: Neuron index in range 0..maxNumberNeurons-1 (after first renumbering)
 * Value: Node struct tracking connectivity statistics
 *
 * Used during genome→neural net conversion to track which neurons are referenced
 * and determine which can be culled.
 */
typedef std::map<uint16_t, Node> NodeMap;

/**
 * @typedef ConnectionList
 * @brief Temporary list of Gene connections during network construction
 *
 * Mutable list used during wiring process. Connections may be removed during
 * neuron culling, then final connections are copied to Individual::nnet.connections.
 */
typedef std::list<Gene> ConnectionList;

/**
 * @brief Generates a random gene with randomized fields
 *
 * Creates a single Gene with all fields randomly initialized:
 * - sourceType: SENSOR (1) or NEURON (0)
 * - sourceNum: Random 15-bit unsigned value (0..0x7fff)
 * - sinkType: ACTION (1) or NEURON (0)
 * - sinkNum: Random 15-bit unsigned value (0..0x7fff)
 * - weight: Random weight via Gene::makeRandomWeight()
 *
 * @note Neuron indices are 15-bit to avoid sign bit issues; remapped later
 *       via modulo to actual neuron count
 *
 * @return Newly created random Gene
 *
 * @todo Remove hardcoded bit widths; use Gene member properties instead
 * @see makeRandomGenome() for genome-level randomization
 */
Gene makeRandomGene() {
  Gene gene;

  gene.sourceType = randomUint() & 1;
  gene.sourceNum = (uint16_t)randomUint(0, 0x7fff);
  gene.sinkType = randomUint() & 1;
  gene.sinkNum = (uint16_t)randomUint(0, 0x7fff);
  gene.weight = Gene::makeRandomWeight();

  return gene;
}

/**
 * @brief Generates a random genome with variable length
 *
 * Creates a genome containing random genes with length determined by config:
 * - Length range: [genomeInitialLengthMin, genomeInitialLengthMax]
 * - Each gene is randomly generated via makeRandomGene()
 *
 * Used during initial population spawning to create genetic diversity.
 *
 * @return Newly created random Genome (vector of Genes)
 *
 * @see makeRandomGene() for individual gene creation
 * @see Params::genomeInitialLengthMin, Params::genomeInitialLengthMax
 */
Genome makeRandomGenome() {
  Genome genome;

  unsigned length =
      randomUint(parameterMngrSingleton.genomeInitialLengthMin, parameterMngrSingleton.genomeInitialLengthMax);
  for (unsigned n = 0; n < length; ++n) {
    genome.push_back(makeRandomGene());
  }

  return genome;
}

/**
 * @brief Converts genome to renumbered connection list (first remapping stage)
 *
 * Performs initial renumbering of all gene indices to valid ranges:
 * - **Neurons**: 16-bit genome indices → 0..maxNumberNeurons-1 (via modulo)
 * - **Sensors**: Remapped to 0..NUM_SENSES-1 (via modulo)
 * - **Actions**: Remapped to 0..NUM_ACTIONS-1 (via modulo)
 *
 * This is the first of two renumbering stages. The second stage (sequential
 * renumbering 0..N) occurs after culling useless neurons.
 *
 * ## Example
 * ```
 * Genome gene: sourceNum=50000, sinkNum=30000 (both NEURON type)
 * If maxNumberNeurons=128:
 *   → sourceNum = 50000 % 128 = 16
 *   → sinkNum   = 30000 % 128 = 48
 * ```
 *
 * @param[out] connectionList Output list populated with renumbered genes
 * @param[in] genome Input genome to convert
 *
 * @note Clears connectionList before populating
 * @see cullUselessNeurons() for second renumbering after culling
 */
void makeRenumberedConnectionList(ConnectionList& connectionList, const Genome& genome) {
  connectionList.clear();
  for (auto const& gene : genome) {
    connectionList.push_back(gene);
    auto& conn = connectionList.back();

    if (conn.sourceType == NEURON) {
      conn.sourceNum %= parameterMngrSingleton.maxNumberNeurons;
    } else {
      conn.sourceNum %= Sensor::NUM_SENSES;
    }

    if (conn.sinkType == NEURON) {
      conn.sinkNum %= parameterMngrSingleton.maxNumberNeurons;
    } else {
      conn.sinkNum %= Action::NUM_ACTIONS;
    }
  }
}

/**
 * @brief Builds node map from connection list with connectivity statistics
 *
 * Scans all connections to discover neurons and track their connectivity:
 * - Creates Node entry for each neuron referenced as source or sink
 * - Counts outputs (connections where neuron is source)
 * - Counts self-inputs (connections where source == sink neuron)
 * - Counts external inputs (connections from sensors or other neurons)
 *
 * ## Connectivity Tracking
 * For each connection:
 * - **Sink is NEURON**: Increment that neuron's input counters
 * - **Source is NEURON**: Increment that neuron's output counter
 * - **Self-connection**: Increment numSelfInputs specifically
 *
 * @param[out] nodeMap Output map populated with neuron indices and metadata
 * @param[in] connectionList Input connections (already renumbered)
 *
 * @note Clears nodeMap before populating
 * @note Only creates entries for neurons actually referenced in connections
 * @see Node for structure of connectivity metadata
 */
void makeNodeList(NodeMap& nodeMap, const ConnectionList& connectionList) {
  nodeMap.clear();

  for (const Gene& conn : connectionList) {
    if (conn.sinkType == NEURON) {
      auto it = nodeMap.find(conn.sinkNum);
      if (it == nodeMap.end()) {
        assert(conn.sinkNum < parameterMngrSingleton.maxNumberNeurons);
        nodeMap.insert(std::pair<uint16_t, Node>(conn.sinkNum, {}));
        it = nodeMap.find(conn.sinkNum);
        assert(it->first < parameterMngrSingleton.maxNumberNeurons);
        it->second.numOutputs = 0;
        it->second.numSelfInputs = 0;
        it->second.numInputsFromSensorsOrOtherNeurons = 0;
      }

      if (conn.sourceType == NEURON && (conn.sourceNum == conn.sinkNum)) {
        ++(it->second.numSelfInputs);
      } else {
        ++(it->second.numInputsFromSensorsOrOtherNeurons);
      }
      assert(nodeMap.count(conn.sinkNum) == 1);
    }
    if (conn.sourceType == NEURON) {
      auto it = nodeMap.find(conn.sourceNum);
      if (it == nodeMap.end()) {
        assert(conn.sourceNum < parameterMngrSingleton.maxNumberNeurons);
        nodeMap.insert(std::pair<uint16_t, Node>(conn.sourceNum, {}));
        it = nodeMap.find(conn.sourceNum);
        assert(it->first < parameterMngrSingleton.maxNumberNeurons);
        it->second.numOutputs = 0;
        it->second.numSelfInputs = 0;
        it->second.numInputsFromSensorsOrOtherNeurons = 0;
      }
      ++(it->second.numOutputs);
      assert(nodeMap.count(conn.sourceNum) == 1);
    }
  }
}

/**
 * @brief Removes all connections feeding a specific neuron
 *
 * Called during neuron culling to clean up connections to neurons being removed.
 * For each removed connection:
 * - If source is another neuron, decrement that neuron's output count
 * - Remove connection from list
 *
 * This ensures connectivity statistics remain accurate as neurons are culled.
 *
 * @param[in,out] connections Connection list to modify
 * @param[in,out] nodeMap Node map to update (output counts decremented)
 * @param[in] neuronNumber Index of neuron whose input connections should be removed
 *
 * @note Uses erase-remove idiom for safe list modification during iteration
 * @see cullUselessNeurons() for the calling context
 */
void removeConnectionsToNeuron(ConnectionList& connections, NodeMap& nodeMap, uint16_t neuronNumber) {
  for (auto itConn = connections.begin(); itConn != connections.end();) {
    if (itConn->sinkType == NEURON && itConn->sinkNum == neuronNumber) {
      /// Remove the connection. If the connection source is from another
      /// neuron, also decrement the other neuron's numOutputs:
      if (itConn->sourceType == NEURON) {
        --(nodeMap[itConn->sourceNum].numOutputs);
      }
      itConn = connections.erase(itConn);
    } else {
      ++itConn;
    }
  }
}

/**
 * @brief Iteratively removes neurons with no functional outputs
 *
 * Culls neurons that don't contribute to the network's action outputs:
 * - **No outputs**: numOutputs == 0
 * - **Only self-connections**: numOutputs == numSelfInputs
 *
 * ## Iterative Process
 * The algorithm runs multiple passes because removing a neuron may cause
 * cascading effects:
 * 1. Find neuron with zero functional outputs
 * 2. Remove all connections feeding that neuron (via removeConnectionsToNeuron)
 * 3. Decrement output counts for source neurons of removed connections
 * 4. Repeat until no more neurons can be culled
 *
 * ## Example Cascade
 * ```
 * Initial: A→B→C (B has 1 output to C)
 * Step 1: C culled (no outputs) → connection B→C removed
 * Step 2: B now has no outputs → B culled → connection A→B removed
 * Result: Only A remains (if it has other outputs)
 * ```
 *
 * @param[in,out] connections Connection list (connections to culled neurons removed)
 * @param[in,out] nodeMap Node map (culled neurons removed)
 *
 * @note Critical for network efficiency; prevents wasted computation on useless neurons
 * @see removeConnectionsToNeuron() for connection cleanup logic
 */
void cullUselessNeurons(ConnectionList& connections, NodeMap& nodeMap) {
  bool allDone = false;
  while (!allDone) {
    allDone = true;
    for (auto itNeuron = nodeMap.begin(); itNeuron != nodeMap.end();) {
      assert(itNeuron->first < parameterMngrSingleton.maxNumberNeurons);
      /// We're looking for neurons with zero outputs, or neurons that feed
      /// itself and nobody else:
      if (itNeuron->second.numOutputs == itNeuron->second.numSelfInputs) {  ///< could be 0
        allDone = false;
        /// Find and remove connections from sensors or other neurons
        removeConnectionsToNeuron(connections, nodeMap, itNeuron->first);
        itNeuron = nodeMap.erase(itNeuron);
      } else {
        ++itNeuron;
      }
    }
  }
}

/**
 * @brief Converts individual's genome into functional neural network wiring
 *
 * Main entry point for genome→neural net translation, called when agent spawns.
 * Transforms genetic encoding into executable neural network structure optimized
 * for feedforward execution.
 *
 * ## Pipeline Stages
 * 1. **Renumbering**: Map genome indices to valid ranges (makeRenumberedConnectionList)
 * 2. **Node Discovery**: Identify all neurons and track connectivity (makeNodeList)
 * 3. **Culling**: Remove useless neurons iteratively (cullUselessNeurons)
 * 4. **Remapping**: Assign sequential 0-based indices to surviving neurons
 * 5. **Wiring**: Build final connection list with optimized ordering
 * 6. **Neuron Creation**: Initialize neuron state array
 *
 * ## Connection Ordering Optimization
 * Connections are ordered for efficient feedforward processing:
 * - **Phase 1**: Sensor/Neuron → Neuron connections (internal processing)
 * - **Phase 2**: Sensor/Neuron → Action connections (outputs)
 *
 * This allows feedForward() to process all internal neurons before actions.
 *
 * ## Neuron State
 * Each surviving neuron is initialized with:
 * - `output`: Set to initialNeuronOutput() value
 * - `driven`: True if neuron receives external inputs (not just self-connections)
 *
 * @note Member function of Individual; modifies this->nnet
 * @note Genome preserved unchanged; only nnet is built
 *
 * @see makeRenumberedConnectionList() for stage 1
 * @see cullUselessNeurons() for stage 3
 * @see feedForward() in feedForward.cpp for execution using this wiring
 */

}  // namespace Genetics
namespace Agents {

void Individual::createWiringFromGenome() {
  Genetics::NodeMap nodeMap;                ///< list of neurons and their number of inputs and outputs
  Genetics::ConnectionList connectionList;  ///< synaptic connections

  /// Convert the indiv's genome to a renumbered connection list
  Genetics::makeRenumberedConnectionList(connectionList, genome);

  /// Make a node (neuron) list from the renumbered connection list
  Genetics::makeNodeList(nodeMap, connectionList);

  /// Find and remove neurons that don't feed anything or only feed themself.
  /// This reiteratively removes all connections to the useless neurons.
  Genetics::cullUselessNeurons(connectionList, nodeMap);

  /// The neurons map now has all the referenced neurons, their neuron numbers,
  /// and the number of outputs for each neuron. Now we'll renumber the neurons
  /// starting at zero.

  assert(nodeMap.size() <= Simulation::parameterMngrSingleton.maxNumberNeurons);
  uint16_t newNumber = 0;
  for (auto& node : nodeMap) {
    assert(node.second.numOutputs != 0);
    node.second.remappedNumber = newNumber++;
  }

  /// Create the indiv's connection list in two passes:
  /// First the connections to neurons, then the connections to actions.
  /// This ordering optimizes the feed-forward function in feedForward.cpp.

  nnet.connections.clear();

  /// First, the connections from sensor or neuron to a neuron
  for (auto const& conn : connectionList) {
    if (conn.sinkType == Genetics::NEURON) {
      nnet.connections.push_back(conn);
      auto& newConn = nnet.connections.back();
      /// fix the destination neuron number
      newConn.sinkNum = nodeMap[newConn.sinkNum].remappedNumber;
      /// if the source is a neuron, fix its number too
      if (newConn.sourceType == Genetics::NEURON) {
        newConn.sourceNum = nodeMap[newConn.sourceNum].remappedNumber;
      }
    }
  }

  /// Last, the connections from sensor or neuron to an action
  for (auto const& conn : connectionList) {
    if (conn.sinkType == Genetics::ACTION) {
      nnet.connections.push_back(conn);
      auto& newConn = nnet.connections.back();
      /// if the source is a neuron, fix its number
      if (newConn.sourceType == Genetics::NEURON) {
        newConn.sourceNum = nodeMap[newConn.sourceNum].remappedNumber;
      }
    }
  }

  /// Create the indiv's neural node list
  nnet.neurons.clear();
  for (unsigned neuronNum = 0; neuronNum < nodeMap.size(); ++neuronNum) {
    nnet.neurons.push_back({});
    nnet.neurons.back().output = Genetics::initialNeuronOutput();
    nnet.neurons.back().driven = (nodeMap[neuronNum].numInputsFromSensorsOrOtherNeurons != 0);
  }
}

}  // namespace Agents
namespace Genetics {

// =============================================================================
// Genetic Mutation and Reproduction Functions
// =============================================================================

/**
 * @brief Applies a single-bit mutation to a random gene in the genome
 *
 * Performs point mutation by flipping bits in gene fields. Two methods available:
 *
 * ## Method 0 (Disabled)
 * Random byte-level bit flip in genome's raw memory representation
 *
 * ## Method 1 (Active)
 * Structured field-level mutations with probabilities:
 * - **20% chance**: Flip sourceType (SENSOR ↔ NEURON)
 * - **20% chance**: Flip sinkType (ACTION ↔ NEURON)
 * - **20% chance**: Flip random bit in sourceNum
 * - **20% chance**: Flip random bit in sinkNum
 * - **20% chance**: Flip random bit (1..15) in weight
 *
 * Method 1 preserves gene structure better than raw byte manipulation.
 *
 * @param[in,out] genome Genome to mutate (one gene modified)
 *
 * @note Weight bit flips exclude bit 0 to avoid tiny changes
 * @see applyPointMutations() for multiple mutation applications
 */
void randomBitFlip(Genome& genome) {
  int method = 1;

  unsigned byteIndex = randomUint(0, genome.size() - 1) * sizeof(Gene);
  unsigned elementIndex = randomUint(0, genome.size() - 1);
  uint8_t bitIndex8 = 1 << randomUint(0, 7);

  if (method == 0) {
    ((uint8_t*)&genome[0])[byteIndex] ^= bitIndex8;
  } else if (method == 1) {
    float chance = randomUint() / (float)RANDOM_UINT_MAX;  ///< 0..1
    if (chance < 0.2) {                                    ///< sourceType
      genome[elementIndex].sourceType ^= 1;
    } else if (chance < 0.4) {  ///< sinkType
      genome[elementIndex].sinkType ^= 1;
    } else if (chance < 0.6) {  ///< sourceNum
      genome[elementIndex].sourceNum ^= bitIndex8;
    } else if (chance < 0.8) {  ///< sinkNum
      genome[elementIndex].sinkNum ^= bitIndex8;
    } else {  ///< weight
      genome[elementIndex].weight ^= (1 << randomUint(1, 15));
    }
  } else {
    assert(false);
  }
}

/**
 * @brief Trims genome to specified maximum length
 *
 * Removes genes from front or back (50% probability each) to achieve target
 * length. Used during sexual reproduction to manage offspring genome size.
 *
 * ## Trimming Strategy
 * - If genome.size() > length: Remove excess genes
 * - 50% chance: Trim from front (erase beginning genes)
 * - 50% chance: Trim from back (erase ending genes)
 * - Minimum: Always preserve at least 1 gene
 *
 * Only active when simulator configured for variable genome lengths
 * (Params::geneInsertionDeletionRate > 0).
 *
 * @param[in,out] genome Genome to trim
 * @param[in] length Target maximum length
 *
 * @note Does nothing if genome.size() <= length or length == 0
 * @see generateChildGenome() for usage during reproduction
 */
void cropLength(Genome& genome, unsigned length) {
  if (genome.size() > length && length > 0) {
    if (randomUint() / (float)RANDOM_UINT_MAX < 0.5) {
      /// trim front
      unsigned numberElementsToTrim = genome.size() - length;
      genome.erase(genome.begin(), genome.begin() + numberElementsToTrim);
    } else {
      /// trim back
      genome.erase(genome.end() - (genome.size() - length), genome.end());
    }
  }
}

/**
 * @brief Randomly inserts or deletes a gene from the genome
 *
 * Applies insertion/deletion mutations with probability controlled by
 * Params::geneInsertionDeletionRate. Deletion vs insertion ratio controlled
 * by Params::deletionRatio.
 *
 * ## Mutation Types
 * - **Deletion**: Remove random gene (if genome has >1 gene)
 * - **Insertion**: Append random gene (if genome < genomeMaxLength)
 *
 * ## Probabilities
 * ```
 * P(mutation occurs) = geneInsertionDeletionRate
 * P(deletion | mutation) = deletionRatio
 * P(insertion | mutation) = 1 - deletionRatio
 * ```
 *
 * Only active when simulator configured for variable genome lengths
 * (geneInsertionDeletionRate > 0).
 *
 * @param[in,out] genome Genome to mutate (0 or 1 gene changed)
 *
 * @note Currently appends insertions; commented code shows random position insertion
 * @see Params::geneInsertionDeletionRate, Params::deletionRatio, Params::genomeMaxLength
 */
void randomInsertDeletion(Genome& genome) {
  float probability = parameterMngrSingleton.geneInsertionDeletionRate;
  if (randomUint() / (float)RANDOM_UINT_MAX < probability) {
    if (randomUint() / (float)RANDOM_UINT_MAX < parameterMngrSingleton.deletionRatio) {
      /// deletion
      if (genome.size() > 1) {
        genome.erase(genome.begin() + randomUint(0, genome.size() - 1));
      }
    } else if (genome.size() < parameterMngrSingleton.genomeMaxLength) {
      /// insertion
      /// genome.insert(genome.begin() + randomUint(0, genome.size() - 1),
      /// makeRandomGene());
      genome.push_back(makeRandomGene());
    }
  }
}

/**
 * @brief Applies multiple point mutations across genome
 *
 * Iterates through all genes in genome, applying randomBitFlip() mutation
 * to each gene independently with probability Params::pointMutationRate.
 *
 * ## Expected Mutations
 * Expected number of mutations = genome.size() × pointMutationRate
 * ```
 * Example: 100-gene genome with pointMutationRate=0.01
 *   → Expect ~1 mutation per genome on average
 * ```
 *
 * @param[in,out] genome Genome to mutate (0+ genes modified)
 *
 * @note Each gene evaluated independently; possible to have 0 or many mutations
 * @see randomBitFlip() for individual gene mutation mechanism
 * @see Params::pointMutationRate for probability configuration
 */
void applyPointMutations(Genome& genome) {
  unsigned numberOfGenes = genome.size();
  while (numberOfGenes-- > 0) {
    if ((randomUint() / (float)RANDOM_UINT_MAX) < parameterMngrSingleton.pointMutationRate) {
      randomBitFlip(genome);
    }
  }
}

/**
 * @brief Generates offspring genome from parent genome(s) with mutations
 *
 * Creates child genome through reproduction (sexual or asexual) followed by
 * mutation. Parent selection can be random or fitness-based depending on
 * configuration.
 *
 * ## Parent Selection
 * **Random Selection** (chooseParentsByFitness=false):
 * - Uniform random choice from all candidates
 *
 * **Fitness-Based Selection** (chooseParentsByFitness=true):
 * - Parents ordered by survival score (computed in survival-criteria.cpp)
 * - Higher-scored parents more likely to be chosen
 * - Selection: parent1 ∈ [1..N-1], parent2 ∈ [0..parent1-1]
 *
 * ## Reproduction Modes
 * **Asexual** (sexualReproduction=false):
 * - Clone random parent genome
 *
 * **Sexual** (sexualReproduction=true):
 * - Start with longer parent genome
 * - Overlay random contiguous slice from shorter parent
 * - Crop to average of parent lengths (±1 gene if odd sum)
 *
 * ## Mutation Pipeline
 * 1. Create base genome (sexual crossover or asexual clone)
 * 2. Apply insertion/deletion mutation (randomInsertDeletion)
 * 3. Apply point mutations (applyPointMutations)
 *
 * @param[in] parentGenomes Vector of candidate parent genomes (ordered by fitness)
 * @return New offspring genome with mutations applied
 *
 * @pre parentGenomes must not be empty
 * @pre Individual genomes must not be empty
 * @post Result genome length ≤ genomeMaxLength
 *
 * @note **Thread Safety**: Must be called in single-thread mode between generations
 * @see Params::sexualReproduction, Params::chooseParentsByFitness
 * @see applyPointMutations(), randomInsertDeletion(), cropLength()
 */
Genome generateChildGenome(const std::vector<Genome>& parentGenomes) {
  /// random parent (or parents if sexual reproduction) with random
  /// mutations
  Genome genome;

  uint16_t parent1Idx;
  uint16_t parent2Idx;

  /// Choose two parents randomly from the candidates. If the parameter
  /// p.chooseParentsByFitness is false, then we choose at random from
  /// all the candidate parents with equal preference. If the parameter is
  /// true, then we give preference to candidate parents according to their
  /// score. Their score was computed by the survival/selection algorithm
  /// in survival-criteria.cpp.
  if (parameterMngrSingleton.chooseParentsByFitness && parentGenomes.size() > 1) {
    parent1Idx = randomUint(1, parentGenomes.size() - 1);
    parent2Idx = randomUint(0, parent1Idx - 1);
  } else {
    parent1Idx = randomUint(0, parentGenomes.size() - 1);
    parent2Idx = randomUint(0, parentGenomes.size() - 1);
  }

  const Genome& g1 = parentGenomes[parent1Idx];
  const Genome& g2 = parentGenomes[parent2Idx];

  if (g1.empty() || g2.empty()) {
    fmt::print("invalid genome\n");
    assert(false);
  }

  auto overlayWithSliceOf = [&](const Genome& gShorter) {
    uint16_t index0 = randomUint(0, gShorter.size() - 1);
    uint16_t index1 = randomUint(0, gShorter.size());
    if (index0 > index1) {
      std::swap(index0, index1);
    }
    std::copy(gShorter.begin() + index0, gShorter.begin() + index1, genome.begin() + index0);
  };

  if (parameterMngrSingleton.sexualReproduction) {
    if (g1.size() > g2.size()) {
      genome = g1;
      overlayWithSliceOf(g2);
      assert(!genome.empty());
    } else {
      genome = g2;
      overlayWithSliceOf(g1);
      assert(!genome.empty());
    }

    /// Trim to length = average length of parents
    unsigned sum = g1.size() + g2.size();
    /// If average length is not an integral number, add one half the time
    if ((sum & 1) && (randomUint() & 1)) {
      ++sum;
    }
    cropLength(genome, sum / 2);
    assert(!genome.empty());
  } else {
    genome = g2;
    assert(!genome.empty());
  }

  randomInsertDeletion(genome);
  assert(!genome.empty());
  applyPointMutations(genome);
  assert(!genome.empty());
  assert(genome.size() <= parameterMngrSingleton.genomeMaxLength);

  return genome;
}

}  // namespace Genetics
}  // namespace Core
}  // namespace v1
}  // namespace BioSim
