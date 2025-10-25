/**
 * @file spawnNewGeneration.cpp
 * @brief Generation spawning and initialization logic for the evolution simulator
 *
 * This file implements the core generation lifecycle functions including:
 * - Initial population generation with random genomes
 * - Subsequent generation spawning from surviving parent genomes
 * - Selection and reproduction logic
 * - Special handling for the altruism challenge with kinship selection
 */

#include "simulator.h"

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iostream>
#include <vector>

namespace BioSim {

extern std::pair<bool, float> passedSurvivalCriterion(const Individual& indiv, unsigned challenge);

/**
 * @brief Initialize generation 0 with random genomes at random locations
 *
 * Creates the first generation of the simulation with completely random genomes.
 * This function is called at the start of a new simulation or when no individuals
 * survive to reproduce.
 *
 * @pre The grid, signals (pheromones), and peeps containers must be pre-allocated
 * @post Grid and signal layers are cleared and reset
 * @post Population is spawned at random locations with random genomes
 * @post Barriers are created according to configured barrier type
 *
 * @note Uses global singletons: grid, pheromones, peeps, parameterMngrSingleton
 * @see initializeNewGeneration() for spawning subsequent generations
 */
void initializeGeneration0() {
  // Clear and reset the grid (already allocated, just reuse it)
  grid.zeroFill();
  grid.createBarrier(parameterMngrSingleton.barrierType);

  // Clear signal layers (already allocated, just reuse them)
  pheromones.zeroFill();

  // Spawn the population with random genomes at random locations
  // Note: peeps container is pre-allocated, indices start at 1
  for (uint16_t index = 1; index <= parameterMngrSingleton.population; ++index) {
    peeps[index].initialize(index, grid.findEmptyLocation(), makeRandomGenome());
  }
}

/**
 * @brief Initialize a new generation from surviving parent genomes
 *
 * Creates a new generation by selecting and mutating genomes from the parent
 * population. Each new individual receives a genome derived from the parent
 * gene pool via generateChildGenome().
 *
 * @param parentGenomes Vector of genomes from individuals who passed survival criteria
 * @param generation Current generation number (used for tracking/logging)
 *
 * @pre The grid, signals (pheromones), and peeps containers must be pre-allocated
 * @pre parentGenomes must contain at least one valid genome
 * @post Grid and signal layers are cleared and reset
 * @post Full population spawned with genomes derived from parentGenomes
 * @post Each individual placed at a random empty location on the grid
 *
 * @note Overwrites all elements in the peeps[] container
 * @note Uses global singletons: grid, pheromones, peeps, parameterMngrSingleton
 * @see spawnNewGeneration() which calls this function
 */
void initializeNewGeneration(const std::vector<Genome>& parentGenomes, unsigned generation) {
  extern Genome generateChildGenome(const std::vector<Genome>& parentGenomes);

  // Clear and reset the grid, signals, and peeps containers (already allocated)
  grid.zeroFill();
  grid.createBarrier(parameterMngrSingleton.barrierType);
  pheromones.zeroFill();

  // Spawn the new population with genomes derived from parents
  // This overwrites all elements of peeps[]
  for (uint16_t index = 1; index <= parameterMngrSingleton.population; ++index) {
    peeps[index].initialize(index, grid.findEmptyLocation(), generateChildGenome(parentGenomes));
  }
}

/**
 * @brief Perform natural selection and spawn the next generation
 *
 * This is the main generation transition function that:
 * 1. Evaluates which individuals passed survival criteria
 * 2. Collects surviving genomes as the parent pool
 * 3. Handles special logic for the altruism challenge (kinship selection)
 * 4. Sorts parents by fitness score
 * 5. Spawns new generation from parent genomes (or random if none survived)
 *
 * The function implements different selection strategies:
 * - Standard challenges: Direct survival criterion evaluation
 * - Altruism challenge: Kin selection where sacrificed individuals can save
 *   related survivors based on genome similarity
 *
 * @param generation Current generation number
 * @param murderCount Number of individuals killed by other individuals this generation
 *
 * @return Number of individuals that survived and will reproduce (parent count)
 *
 * @pre Must be called in single-thread mode between simulation generations
 * @pre Deferred death queue and move queue must be fully processed
 * @post New generation initialized with either parent-derived or random genomes
 * @post Generation statistics logged to epoch log
 *
 * @note Performance consideration: When many individuals survive, rebuilding
 *       all genomes and neural nets is inefficient. Could be optimized to reuse
 *       existing structures with mutations instead of full reconstruction.
 *
 * @warning Thread safety: This function modifies global state and must not be
 *          called concurrently with other simulation operations.
 *
 * @see initializeNewGeneration() for parent-based spawning
 * @see initializeGeneration0() for random spawning when no survivors
 * @see passedSurvivalCriterion() for survival evaluation logic
 */
unsigned spawnNewGeneration(unsigned generation, unsigned murderCount) {
  unsigned sacrificedCount = 0;  ///< Number of individuals in sacrificial area (altruism challenge)

  extern void appendEpochLog(unsigned generation, unsigned numberSurvivors, unsigned murderCount);
  extern std::pair<bool, float> passedSurvivalCriterion(const Individual& indiv, unsigned challenge);
  extern void displaySignalUse();

  // Container holds indexes and survival scores (0.0..1.0) of survivors
  // who will provide genomes for repopulation
  std::vector<std::pair<uint16_t, float>> parents;  ///< <indiv index, score>

  // Container will hold the genomes of the survivors
  std::vector<Genome> parentGenomes;

  if (parameterMngrSingleton.challenge != CHALLENGE_ALTRUISM) {
    // STANDARD CHALLENGES: Direct survival criterion evaluation
    // Build list of individuals who will become parents, saving their scores
    // for later sorting. Indices start at 1.
    for (uint16_t index = 1; index <= parameterMngrSingleton.population; ++index) {
      std::pair<bool, float> passed = passedSurvivalCriterion(peeps[index], parameterMngrSingleton.challenge);
      // Save the parent genome only if it results in valid neural connections
      // @todo Optimization: Could use std::move instead of copy if parents
      //       no longer need their genome, though impact likely negligible
      if (passed.first && !peeps[index].nnet.connections.empty()) {
        parents.push_back({index, passed.second});
      }
    }
  } else {
    // ALTRUISM CHALLENGE: Kin selection with sacrificial and spawning areas
    // Test if agents are in the sacrificial area (give life for others) or
    // spawning area (reproduce). Count sacrifices and save spawners with scores.
    // Indices start at 1.

    bool considerKinship = true;
    std::vector<uint16_t> sacrificesIndexes;  ///< Individuals who gave their lives

    for (uint16_t index = 1; index <= parameterMngrSingleton.population; ++index) {
      // Test for spawning area
      std::pair<bool, float> passed = passedSurvivalCriterion(peeps[index], CHALLENGE_ALTRUISM);
      if (passed.first && !peeps[index].nnet.connections.empty()) {
        parents.push_back({index, passed.second});
      } else {
        // Test for sacrificial area
        passed = passedSurvivalCriterion(peeps[index], CHALLENGE_ALTRUISM_SACRIFICE);
        if (passed.first && !peeps[index].nnet.connections.empty()) {
          if (considerKinship) {
            sacrificesIndexes.push_back(index);
          } else {
            ++sacrificedCount;
          }
        }
      }
    }

    unsigned generationToApplyKinship = 10;
    constexpr unsigned altruismFactor = 10;  ///< Saved:sacrificed ratio (10:1)

    if (considerKinship) {
      if (generation > generationToApplyKinship) {
        // @todo OPTIMIZE: This nested loop approach is O(n^3) - needs improvement
        float threshold = 0.7;  ///< Genome similarity threshold for kin recognition

        std::vector<std::pair<uint16_t, float>> survivingKin;
        // For each sacrifice, allow altruismFactor individuals to survive
        for (unsigned passes = 0; passes < altruismFactor; ++passes) {
          for (uint16_t sacrificedIndex : sacrificesIndexes) {
            // Randomize the search order to avoid repeatedly selecting the same parent
            unsigned startIndex = randomUint(0, parents.size() - 1);
            for (unsigned count = 0; count < parents.size(); ++count) {
              const std::pair<uint16_t, float>& possibleParent = parents[(startIndex + count) % parents.size()];
              const Genome& g1 = peeps[sacrificedIndex].genome;
              const Genome& g2 = peeps[possibleParent.first].genome;
              float similarity = genomeSimilarity(g1, g2);
              if (similarity >= threshold) {
                survivingKin.push_back(possibleParent);
                // @todo Mark this parent so it can't be selected again?
                break;
              }
            }
          }
        }
        std::cout << parents.size() << " passed, " << sacrificesIndexes.size() << " sacrificed, " << survivingKin.size()
                  << " saved" << std::endl;
        parents = std::move(survivingKin);
      }
    } else {
      // Limit the parent list based on sacrifice count
      unsigned numberSaved = sacrificedCount * altruismFactor;
      std::cout << parents.size() << " passed, " << sacrificedCount << " sacrificed, " << numberSaved << " saved"
                << std::endl;
      if (!parents.empty() && numberSaved < parents.size()) {
        parents.erase(parents.begin() + numberSaved, parents.end());
      }
    }
  }

  // Sort parent indices by fitness scores (descending order - highest first)
  std::sort(parents.begin(), parents.end(),
            [](const std::pair<uint16_t, float>& parent1, const std::pair<uint16_t, float>& parent2) {
              return parent1.second > parent2.second;
            });

  // Assemble the list of parent genomes, ordered by fitness scores
  parentGenomes.reserve(parents.size());
  for (const std::pair<uint16_t, float>& parent : parents) {
    parentGenomes.push_back(peeps[parent.first].genome);
  }

  std::cout << "Gen " << generation << ", " << parentGenomes.size() << " survivors" << std::endl;
  appendEpochLog(generation, parentGenomes.size(), murderCount);
  // displaySignalUse(); // Uncomment for debugging signal layer usage

  // At this point we have zero or more parent genomes

  if (!parentGenomes.empty()) {
    // Spawn a new generation from surviving parents
    initializeNewGeneration(parentGenomes, generation + 1);
  } else {
    // Special case: No survivors - restart simulation from scratch
    // with randomly-generated genomes
    initializeGeneration0();
  }

  return parentGenomes.size();
}

}  // namespace BioSim
