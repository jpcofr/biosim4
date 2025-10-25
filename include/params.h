#ifndef BIOSIM4_INCLUDE_PARAMS_H_
#define BIOSIM4_INCLUDE_PARAMS_H_

/**
 * @file params.h
 * @brief Global simulator parameters and configuration
 *
 * ## Adding a New Parameter
 * Follow this 5-step process:
 * 1. Add a member to struct Params in params.h
 * 2. Add default value in ConfigManager::ConfigManager() constructor (configManager.cpp)
 * 3. Add TOML parsing logic in ConfigManager::loadFromToml() (configManager.cpp)
 * 4. Add string-to-value conversion in ConfigManager::setParameterInternal() (configManager.cpp)
 * 5. Add to config/biosim4.toml with documentation (use TOML syntax)
 *
 * @see configManager.h for the modern configuration system
 */

#include <string>

namespace BioSim {

/**
 * @enum RunMode
 * @brief Simulator execution state
 */
enum class RunMode { STOP, RUN, PAUSE, ABORT };

/// Global run mode variable
extern RunMode runMode;

/**
 * @struct Params
 * @brief Configuration parameters for the simulator
 *
 * A global instance of Params is initialized by the simulator() function
 * with configuration from ConfigManager. The simulator and all subsystems
 * access this configuration via the global parameterMngrSingleton reference.
 *
 * @see ConfigManager for configuration loading and management
 * @see parameterMngrSingleton global reference in simulator.h
 */
struct Params {
  /// Population and generation settings
  unsigned population;          ///< Population size (>= 0)
  unsigned stepsPerGeneration;  ///< Steps per generation (> 0)
  unsigned maxGenerations;      ///< Maximum generations to simulate (>= 0)
  unsigned numThreads;          ///< Number of parallel threads (> 0)

  /// Genome and neural network settings
  unsigned signalLayers;      ///< Number of pheromone layers (>= 0)
  unsigned genomeMaxLength;   ///< Maximum genome length (> 0)
  unsigned maxNumberNeurons;  ///< Maximum neurons per individual (> 0)

  /// Mutation settings
  double pointMutationRate;          ///< Point mutation probability (0.0..1.0)
  double geneInsertionDeletionRate;  ///< Gene indel probability (0.0..1.0)
  double deletionRatio;              ///< Deletion vs insertion ratio (0.0..1.0)

  /// Reproduction settings
  bool killEnable;              ///< Enable survival selection
  bool sexualReproduction;      ///< Enable sexual reproduction
  bool chooseParentsByFitness;  ///< Select parents by fitness vs random

  /// Sensor settings
  float populationSensorRadius;         ///< Radius for population sensing (> 0.0)
  unsigned signalSensorRadius;          ///< Radius for signal sensing (> 0)
  float responsiveness;                 ///< Agent responsiveness (>= 0.0)
  unsigned responsivenessCurveKFactor;  ///< Response curve shape (1, 2, 3, or 4)
  unsigned longProbeDistance;           ///< Distance for long-range probes (> 0)
  unsigned shortProbeBarrierDistance;   ///< Distance for barrier detection (> 0)
  float valenceSaturationMag;           ///< Signal saturation magnitude

  /// Video output settings
  bool saveVideo;                 ///< Enable video generation
  unsigned videoStride;           ///< Save every Nth generation (> 0)
  unsigned videoSaveFirstFrames;  ///< Always save first N generations (>= 0, overrides videoStride)
  unsigned displayScale;          ///< Pixel scale for output
  unsigned agentSize;             ///< Visual size of agents

  /// Analysis and logging settings
  unsigned genomeAnalysisStride;    ///< Genome analysis frequency (> 0)
  unsigned displaySampleGenomes;    ///< Number of sample genomes to display (>= 0)
  unsigned genomeComparisonMethod;  ///< 0 = Jaro-Winkler; 1 = Hamming
  bool updateGraphLog;              ///< Enable graph log updates
  unsigned updateGraphLogStride;    ///< Graph log update frequency (> 0)

  /// Challenge and environment settings
  unsigned challenge;    ///< Challenge type identifier
  unsigned barrierType;  ///< Barrier configuration (>= 0)

  /// Random number generation
  bool deterministic;  ///< Use deterministic RNG
  unsigned RNGSeed;    ///< Random number generator seed (>= 0)

  /// Grid dimensions (immutable after initialization)
  uint16_t gridSize_X;                ///< Grid width (2..0x10000)
  uint16_t gridSize_Y;                ///< Grid height (2..0x10000)
  unsigned genomeInitialLengthMin;    ///< Min initial genome length (> 0, < genomeInitialLengthMax)
  unsigned genomeInitialLengthMax;    ///< Max initial genome length (> 0, > genomeInitialLengthMin)
  std::string logDir;                 ///< Directory for log files
  std::string imageDir;               ///< Directory for image/video output
  std::string graphLogUpdateCommand;  ///< Command to update graph logs

  /// Automatic state tracking (not set via config file)
  unsigned parameterChangeGenerationNumber;  ///< Most recent generation with automatic parameter change
};

}  // namespace BioSim

#endif  ///< BIOSIM4_INCLUDE_PARAMS_H_
