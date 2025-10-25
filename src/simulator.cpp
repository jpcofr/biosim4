/**
 * @file simulator.cpp
 * @brief Main simulation loop and thread orchestration
 *
 * This file contains the core simulation engine that drives the evolutionary process.
 * It implements a three-level nested loop structure:
 * - Outer loop: Generations (epochs of evolution)
 * - Middle loop: Simulation steps within each generation
 * - Inner loop: Individual creatures (parallelized via OpenMP)
 *
 * The simulator manages four global singleton data structures:
 * - grid: 2D spatial world where creatures exist
 * - pheromones: Signal layers overlaying the grid
 * - peeps: Container of all Individual creatures
 * - imageWriter: Video frame capture system
 *
 * Thread safety is achieved through a deferred execution model where mutations
 * to shared state (movements, deaths, signal updates) are queued during parallel
 * execution and applied in single-threaded sections.
 *
 * @see simulator() for the main entry point
 * @see simulationStepOneIndividual() for per-creature execution
 */

#include "simulator.h"

#include "imageWriter.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <filesystem>
#include <iostream>
#include <utility>

/**
 * @namespace BioSim
 * @brief Core namespace for the biological evolution simulator
 */
namespace BioSim {

// Forward declarations of functions implemented in other compilation units

/// @brief Initialize the starting population with random genomes and positions
extern void initializeGeneration0();

/**
 * @brief Create the next generation from survivors
 * @param generation Current generation number
 * @param murderCount Number of individuals that died during the generation
 * @return Number of survivors who reproduced
 */
extern unsigned spawnNewGeneration(unsigned generation, unsigned murderCount);

/**
 * Display genome information for debugging
 * @see Implementation in analysis.cpp
 */
extern void displaySampleGenomes(unsigned count);

/**
 * Execute the actions computed by an individual's neural network
 * @see Full documentation in executeActions.cpp
 */
extern void executeActions(Individual& indiv, std::array<float, Action::NUM_ACTIONS>& actionLevels);

/**
 * End-of-step housekeeping (apply queued actions, update signals)
 * @see Implementation in endOfSimStep.cpp
 */
extern void endOfSimulationStep(unsigned simStep, unsigned generation);

/**
 * End-of-generation processing (video output, logging, cleanup)
 * @see Implementation in endOfGeneration.cpp
 */
extern void endOfGeneration(unsigned generation);

/// @brief Global simulation run state (RUN, PAUSE, STOP)
RunMode runMode = RunMode::STOP;

/**
 * @brief Global 2D grid representing the spatial world
 *
 * The grid stores 16-bit indices identifying which creature occupies each location.
 * Index 0 represents an empty cell. Valid creature indices start at 1.
 * Grid coordinates are (x, y) with origin at top-left.
 *
 * @note Thread-safe as read-only during parallel simulation steps
 */
Grid grid;

/**
 * @brief Global pheromone signal layers overlaying the grid
 *
 * Multi-layer 2D array where creatures can emit and sense chemical signals.
 * Each layer holds uint8 values (0-255) representing signal strength.
 * Signals diffuse and fade over time according to configured parameters.
 *
 * @note Thread-safe: threads only write to their creature's location via
 * signals.increment(), read-only elsewhere
 */
Signals pheromones;

/**
 * @brief Global container of all individuals in the population
 *
 * Indexed collection of Individual structs. Index 0 is reserved (unused).
 * Valid indices range from 1 to population size. Each Individual contains
 * genome, neural network, position, age, and state.
 *
 * @note Thread-safe: each thread modifies only its assigned individual,
 * reads other individuals as read-only
 */
Peeps peeps;

/**
 * @brief Global video frame writer for simulation visualization
 *
 * Captures periodic snapshots of the grid state and compiles them into
 * video files at the end of each generation. Currently operates in
 * synchronous mode due to threading issues (see IMAGEWRITER_INTEGRATION_GUIDE.md).
 */
ImageWriter imageWriter;

/**
 * @brief Global parameter manager maintaining simulation configuration
 *
 * Loads settings from biosim4.ini and monitors for runtime changes.
 * Maintains an internal Params struct with validated parameter values.
 * Access via getParamRef() returns read-only reference.
 */
ParamManager paramManager;

/**
 * @brief Read-only singleton reference to current parameter values
 *
 * Convenience reference to paramManager's internal Params struct.
 * Updated automatically when paramManager reloads configuration.
 * Use this instead of repeatedly calling getParamRef().
 */
const Params& parameterMngrSingleton{paramManager.getParamRef()};

/**
 * @brief Execute one simulation step for a single individual
 *
 * This function is invoked in parallel for each living creature during a
 * simulation step. It performs a complete sense-think-act cycle:
 *
 * 1. Increment the individual's age counter
 * 2. Feed sensor inputs through the neural network (feedForward)
 * 3. Queue actions for deferred execution (executeActions)
 *
 * Actions like movement, death, and signal emission are queued rather than
 * executed immediately. This deferred execution model allows all threads to
 * treat shared data structures (grid, signals, peeps) as read-only during
 * parallel execution. Queued actions are applied in single-threaded sections
 * at the end of each simulation step.
 *
 * **Thread Safety:**
 * - `grid`: Read-only access (query occupied locations, barriers)
 * - `pheromones`: Read-only except signals.increment() at creature's location
 * - `peeps`: Read-write for this individual, read-only for others
 * - `randomUint`: Thread-local instance (seeded per-thread in parallel region)
 *
 * **Performance Note:**
 * This function is the computational hotspot of the simulator. It's called
 * population × stepsPerGeneration times per generation and is parallelized
 * via OpenMP's `#pragma omp for` directive.
 *
 * @param individual Reference to the Individual struct being simulated
 * @param simulationStep Current step within generation (0 to stepsPerGeneration-1)
 *
 * @see Individual::feedForward() for neural network execution
 * @see executeActions() for action queue processing
 * @see endOfSimulationStep() for deferred action application
 */
void simulationStepOneIndividual(Individual& individual, unsigned simulationStep) {
  ++individual.age;
  auto actionLevels = individual.feedForward(simulationStep);
  executeActions(individual, actionLevels);
}

/**
 * @brief Main simulation loop - top-level entry point for the evolutionary simulator
 *
 * This function orchestrates the entire simulation lifecycle through a three-level
 * nested loop structure:
 *
 * **Outer Loop (Generations):**
 * - Each generation begins with a population of creatures
 * - Creatures live for a fixed number of simulation steps
 * - At generation end, selection pressure is applied (survival criteria)
 * - Survivors reproduce to create the next generation
 * - Failed generations (no survivors) restart from generation 0
 *
 * **Middle Loop (Simulation Steps):**
 * - Fixed duration per generation (stepsPerGeneration parameter)
 * - Each step: all living creatures sense, think, and queue actions
 * - Deaths occur during steps but corpses persist until generation end
 * - Step concludes with single-threaded action resolution
 *
 * **Inner Loop (Individuals):**
 * - Parallelized via OpenMP across available threads
 * - Each thread executes simulationStepOneIndividual() for assigned creatures
 * - Thread-safe through read-only data access and deferred action queues
 *
 * **Initialization Sequence:**
 * 1. Display available sensors and actions (printSensorsActions)
 * 2. Load configuration from biosim4.ini (paramManager)
 * 3. Initialize random number generator (randomUint)
 * 4. Initialize global singletons (grid, pheromones, imageWriter, peeps)
 * 5. Create generation 0 with random genomes and placements
 *
 * **Global Data Structures:**
 * - `grid`: 2D spatial world (16-bit indices, 0=empty, 1..N=creature IDs)
 * - `pheromones`: Multi-layer signal grid (uint8 values, diffusion/fade)
 * - `peeps`: Indexed creature container (index 0 reserved, 1..population valid)
 * - `imageWriter`: Video frame capture system (synchronous mode)
 * - `paramManager`: Configuration management with hot-reload support
 *
 * **Thread Architecture:**
 * - Main thread: Orchestrates loops, applies queued actions, I/O operations
 * - Worker threads: Parallel execution of simulationStepOneIndividual()
 * - Thread count: Configurable via numThreads parameter (OpenMP)
 *
 * **Configuration Hot-Reload:**
 * The paramManager checks biosim4.ini for changes at each generation boundary.
 * Most parameters can be modified at runtime without restarting the simulation.
 *
 * **Video Generation:**
 * When enabled (saveVideo=true), frames are captured periodically and compiled
 * into .avi movies at generation end. See imageWriter and IMAGEWRITER_INTEGRATION_GUIDE.md.
 *
 * **Genome Analysis:**
 * Sample genomes are displayed to stdout at intervals (genomeAnalysisStride).
 * Pipe output to tools/graph-nnet.py for neural network visualization.
 *
 * @param argc Command-line argument count
 * @param argv Command-line arguments (argv[1] = optional config file path)
 *
 * @note Default config file: "config/biosim4.ini"
 * @note This function does not return until maxGenerations is reached or runMode changes
 *
 * @see simulationStepOneIndividual() for per-creature execution
 * @see endOfSimulationStep() for action queue resolution
 * @see endOfGeneration() for generation boundary processing
 * @see spawnNewGeneration() for reproduction logic
 * @see initializeGeneration0() for initial population creation
 */
void simulator(int argc, char** argv) {
  // Display available sensors and actions for debugging/verification
  printSensorsActions();

  // Initialize parameter system with defaults, then load from config file
  paramManager.setDefaults();
  const char* configFile = argc > 1 ? argv[1] : "config/biosim4.ini";
  paramManager.registerConfigFile(configFile);
  paramManager.updateFromConfigFile(0);
  paramManager.checkParameters();  // Validate and report configuration issues

  // Seed the global random number generator (per-thread instances seeded later)
  randomUint.initialize();

  // Initialize global singleton data structures with configured dimensions
  grid.initialize(parameterMngrSingleton.gridSize_X, parameterMngrSingleton.gridSize_Y);
  pheromones.initialize(parameterMngrSingleton.signalLayers, parameterMngrSingleton.gridSize_X,
                        parameterMngrSingleton.gridSize_Y);
  imageWriter.init(parameterMngrSingleton.signalLayers, parameterMngrSingleton.gridSize_X,
                   parameterMngrSingleton.gridSize_Y);
  peeps.initialize(parameterMngrSingleton.population);

  // Create the initial population with random genomes and positions
  unsigned currentGeneration = 0;
  initializeGeneration0();
  runMode = RunMode::RUN;
  unsigned murderCount;  // Tracks deaths during generation (for logging)

  // OpenMP parallel region: shared data is read-only, mutations via deferred queues
#pragma omp parallel num_threads(parameterMngrSingleton.numThreads) default(shared)
  {
    // Each thread initializes its own random number generator instance
    randomUint.initialize();

    // Outer loop: iterate through generations until stopping condition
    while (runMode == RunMode::RUN && currentGeneration < parameterMngrSingleton.maxGenerations) {
      // Reset death counter for this generation (single-threaded initialization)
#pragma omp single
      murderCount = 0;

      // Middle loop: fixed number of simulation steps per generation
      for (unsigned simulationStep = 0; simulationStep < parameterMngrSingleton.stepsPerGeneration; ++simulationStep) {
        // Inner loop (parallelized): execute one step for each living creature
        // Note: index 0 is reserved in peeps, valid indices start at 1
#pragma omp for schedule(auto)
        for (unsigned individual = 1; individual <= parameterMngrSingleton.population; ++individual)
          if (peeps[individual].alive)
            simulationStepOneIndividual(peeps[individual], simulationStep);

        // Single-threaded section: apply queued actions (movements, deaths, signals)
        // This ensures thread-safe mutation of shared data structures
#pragma omp single
        {
          murderCount += peeps.deathQueueSize();
          endOfSimulationStep(simulationStep, currentGeneration);
        }
      }

      // Single-threaded section: generation boundary processing
#pragma omp single
      {
        // End-of-generation tasks: video output, logging, statistics
        endOfGeneration(currentGeneration);

        // Hot-reload configuration file (allows runtime parameter changes)
        paramManager.updateFromConfigFile(currentGeneration + 1);

        // Apply selection pressure and create next generation from survivors
        unsigned numberSurvivors = spawnNewGeneration(currentGeneration, murderCount);

        // Periodically display sample genomes for analysis/debugging
        if (numberSurvivors > 0 && (currentGeneration % parameterMngrSingleton.genomeAnalysisStride == 0))
          displaySampleGenomes(parameterMngrSingleton.displaySampleGenomes);

        // Restart from generation 0 if population went extinct
        if (numberSurvivors == 0)
          currentGeneration = 0;
        else
          ++currentGeneration;
      }
    }
  }

  // Final genome report for debugging/analysis
  displaySampleGenomes(3);

  std::cout << "Simulator exit." << std::endl;
}

}  // namespace BioSim
