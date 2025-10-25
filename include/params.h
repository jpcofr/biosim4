#ifndef BIOSIM4_INCLUDE_PARAMS_H_
#define BIOSIM4_INCLUDE_PARAMS_H_

/**
 * @file params.h
 * @brief Global simulator parameters and configuration management
 *
 * ## Adding a New Parameter
 * Follow this 4-step process:
 * 1. Add a member to struct Params in params.h
 * 2. Add member and default value to privParams in ParamManager::setDefault() (params.cpp)
 * 3. Add an else clause to ParamManager::ingestParameter() (params.cpp)
 * 4. Add a line to the user's parameter file (default: config/biosim4.ini)
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
 * A private copy of Params is initialized by ParamManager::init(), then
 * modified by UI events via ParamManager::uiMonitor(). The main simulator thread
 * can get an updated, read-only, stable snapshot of Params with
 * ParamManager::paramsSnapshot.
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

/**
 * @class ParamManager
 * @brief Manages parameter configuration and updates
 *
 * Provides centralized parameter management with config file monitoring
 * and runtime updates.
 */
class ParamManager {
 public:
  /**
   * @brief Construct manager with default parameters loaded.
   *
   * Ensures callers (including unit tests) see sensible values even if they
   * never call setDefaults() explicitly.
   */
  ParamManager();

  /**
   * @brief Get read-only reference to current parameters
   * @return const reference to Params
   */
  const Params& getParamRef() const { return privParams; }

  /**
   * @brief Initialize parameters with default values
   */
  void setDefaults();

  /**
   * @brief Register a configuration file for monitoring
   * @param filename Path to config file
   */
  void registerConfigFile(const char* filename);

  /**
   * @brief Update parameters from config file
   * @param generationNumber Current generation number
   */
  void updateFromConfigFile(unsigned generationNumber);

  /**
   * @brief Validate parameter values and constraints
   */
  void checkParameters();

 private:
  Params privParams;           ///< Private parameter storage
  std::string configFilename;  ///< Path to configuration file

  /**
   * @brief Parse and apply a single parameter
   * @param name Parameter name
   * @param val Parameter value as string
   */
  void ingestParameter(std::string name, std::string val);
};

/**
 * @brief Initialize parameters from command-line arguments and config file
 * @param argc Argument count
 * @param argv Argument vector
 * @return Params instance with default values overridden by config file
 *
 * Returns a copy of params with default values overridden by the values
 * in the specified config file. The filename of the config file is saved
 * inside the params for future reference.
 */
Params paramsInit(int argc, char** argv);

}  // namespace BioSim

#endif  ///< BIOSIM4_INCLUDE_PARAMS_H_
