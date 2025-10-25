/// simulator.cpp - Main thread (invoked from main.cpp)
/// This file contains simulator(), the top-level entry point of the simulator.

#include "simulator.h"

#include "imageWriter.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <utility>

/// @brief
namespace BioSim {

extern void initializeGeneration0();
extern unsigned spawnNewGeneration(unsigned generation, unsigned murderCount);
extern void displaySampleGenomes(unsigned count);
extern void executeActions(Individual& indiv, std::array<float, Action::NUM_ACTIONS>& actionLevels);
extern void endOfSimulationStep(unsigned simStep, unsigned generation);
extern void endOfGeneration(unsigned generation);

RunMode runMode = RunMode::STOP;

/// @brief The 2D world where the creatures live
Grid grid;

/// @brief A 2D array of pheromones that overlay the world grid
Signals pheromones;

/// @brief The container of all the individuals in the population
Peeps peeps;

/// @brief Image writer for generating video frames
ImageWriter imageWriter;

/// @brief The paramManager maintains a private copy of the parameter values,
/// and a copy is available read-only singleton p.
ParamManager paramManager;

/// @brief Parameter manager
const Params& parameterMngrSingleton{paramManager.getParamRef()};

/**
 * @brief Execute one simulation step for one individual.
 *
 * This function runs in its own thread, invoked from the main simulator thread.
 * First, indiv.feedForward() is executed to compute action values for later
 * execution. Some actions, like signal emissions (pheromones), agent movement,
 * or deaths, are queued for later execution at the end of the generation in
 * single-threaded mode (deferred queues allow main data structures, e.g., grid,
 * signals, to be accessed read-only in all threads).
 *
 * Thread safety involves the following data structures:
 *   - grid: read-only
 *   - signals: read-write for the agent's location using signals.increment(),
 * read-only elsewhere
 *   - peeps: read-only access to index and genome for other individuals,
 * read-write for our indiv
 *
 * Other important variables:
 *   - simulationStep: current agent age, reset to 0 at each generation start
 * (often matches indiv.age)
 *   - randomUint: global random number generator with a private instance for
 * each thread
 *
 * @param individual: The individual to simulate.
 * @param simulationStep The current simulation step
 */
void simulationStepOneIndividual(Individual& individual, unsigned simulationStep) {
  ++individual.age;
  auto actionLevels = individual.feedForward(simulationStep);
  executeActions(individual, actionLevels);
}

/********************************************************************************
Main simulator thread. This is the top-level entry point of the simulator.

- Agents randomly placed with random genomes initially
- Outer loop: generations; Inner loop: simSteps
- Fixed simSteps for each generation
- Agent deaths occur during simSteps; corpses persist till generation end
- Post-generation: corpses removed, survivors reproduce and die
- Newborns randomly placed; pheromones updated
- SimStep resets; new generation begins

The paramManager manages all the simulator parameters. It starts with defaults,
then keeps them updated as the config file (config/biosim4.ini) changes.

The main simulator-wide data structures are:
    grid    - where the agents live (identified by their non-zero index). 0
means empty. signals - multiple layers overlay the grid, hold pheromones peeps
- an indexed set of agents of type Indiv; indexes start at 1

The important simulator-wide variables are:
    generation - starts at 0, then increments every time the agents die and
reproduce. simStep    - reset to 0 at the start of each generation; fixed number
per generation. randomUint - global random number generator

The threads are:
    main thread       - simulator
    simStepOneIndiv() - child threads created by the main simulator thread
    imageWriter       - saves image frames used to make a movie (possibly not
threaded due to unresolved bugs when threaded)
********************************************************************************/
void simulator(int argc, char** argv) {
  printSensorsActions();  ///< show the agents' capabilities

  paramManager.setDefaults();
  /// Use config directory for biosim4.ini
  const char* configFile = argc > 1 ? argv[1] : "config/biosim4.ini";
  paramManager.registerConfigFile(configFile);
  paramManager.updateFromConfigFile(0);
  paramManager.checkParameters();  ///< check and report any problems

  randomUint.initialize();

  grid.initialize(parameterMngrSingleton.gridSize_X, parameterMngrSingleton.gridSize_Y);
  pheromones.initialize(parameterMngrSingleton.signalLayers, parameterMngrSingleton.gridSize_X,
                        parameterMngrSingleton.gridSize_Y);
  imageWriter.init(parameterMngrSingleton.signalLayers, parameterMngrSingleton.gridSize_X,
                   parameterMngrSingleton.gridSize_Y);
  peeps.initialize(parameterMngrSingleton.population);  ///< the peeps themselves

  unsigned currentGeneration = 0;
  initializeGeneration0();  ///< starting population
  runMode = RunMode::RUN;
  unsigned murderCount;

/// Ensure shared data remains unmodified within parallel regions. Perform
/// modifications in single-threaded sections.
#pragma omp parallel num_threads(parameterMngrSingleton.numThreads) default(shared)
  {
    randomUint.initialize();  ///< seed the RNG, each thread has a private instance

    /// Outer loop: process generations
    while (runMode == RunMode::RUN && currentGeneration < parameterMngrSingleton.maxGenerations) {
#pragma omp single
      murderCount = 0;

      for (unsigned simulationStep = 0; simulationStep < parameterMngrSingleton.stepsPerGeneration; ++simulationStep) {
/// multithreaded loop: index 0 is reserved, start at 1
#pragma omp for schedule(auto)
        for (unsigned individual = 1; individual <= parameterMngrSingleton.population; ++individual)
          if (peeps[individual].alive)
            simulationStepOneIndividual(peeps[individual], simulationStep);

/// In single-thread mode: this executes deferred, queued deaths and movements,
/// updates signal layers (pheromone), etc.
#pragma omp single
        {
          murderCount += peeps.deathQueueSize();
          endOfSimulationStep(simulationStep, currentGeneration);
        }
      }

#pragma omp single
      {
        endOfGeneration(currentGeneration);
        paramManager.updateFromConfigFile(currentGeneration + 1);
        unsigned numberSurvivors = spawnNewGeneration(currentGeneration, murderCount);
        if (numberSurvivors > 0 && (currentGeneration % parameterMngrSingleton.genomeAnalysisStride == 0))
          displaySampleGenomes(parameterMngrSingleton.displaySampleGenomes);

        if (numberSurvivors == 0)
          currentGeneration = 0;
        else
          ++currentGeneration;
      }
    }
  }

  displaySampleGenomes(3);  ///< final report, for debugging

  std::cout << "Simulator exit." << std::endl;
}

}  // namespace BioSim
