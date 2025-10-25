/**
 * @file params.cpp
 * @brief Implementation of parameter management system for BioSim4
 *
 * This file implements the ParamManager class which handles:
 * - Loading and parsing configuration files (biosim4.ini)
 * - Setting default parameter values
 * - Validating parameter types and ranges
 * - Supporting generation-specific parameter changes via `@N` syntax
 *
 * @see params.h for the 4-step process to add new parameters
 */

#include "params.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <string>

/**
 * @note Adding a New Parameter (4-step process):
 *    1. Add a member to struct Params in params.h
 *    2. Add a member and its default value to privParams in ParamManager::setDefaults()
 *          in params.cpp
 *    3. Add an else clause to ParamManager::ingestParameter() in params.cpp
 *    4. Add a line to the user's parameter file (default name config/biosim4.ini)
 */

namespace BioSim {

/**
 * @brief Constructs ParamManager and initializes all parameters to defaults
 *
 * The constructor calls setDefaults() to populate privParams with
 * hardcoded default values before any config file is loaded.
 */
ParamManager::ParamManager() {
  setDefaults();
}

/**
 * @brief Sets all parameters to their default values
 *
 * This method initializes every member of privParams to a sensible default.
 * These defaults are overridden by values from the config file when
 * updateFromConfigFile() is called.
 *
 * @note When adding a new parameter, add its default value here (step 2)
 */
void ParamManager::setDefaults() {
  privParams.gridSize_X = 128;
  privParams.gridSize_Y = 128;
  privParams.challenge = 6;

  privParams.genomeInitialLengthMin = 24;
  privParams.genomeInitialLengthMax = 24;
  privParams.genomeMaxLength = 300;
  privParams.logDir = "./output/logs/";
  privParams.imageDir = "./output/images/";
  privParams.population = 3000;
  privParams.stepsPerGeneration = 300;
  privParams.maxGenerations = 200000;
  privParams.barrierType = 0;
  privParams.numThreads = 4;
  privParams.signalLayers = 1;
  privParams.maxNumberNeurons = 5;
  privParams.pointMutationRate = 0.001;
  privParams.geneInsertionDeletionRate = 0.0;
  privParams.deletionRatio = 0.5;
  privParams.killEnable = false;
  privParams.sexualReproduction = true;
  privParams.chooseParentsByFitness = true;
  privParams.populationSensorRadius = 2.5;
  privParams.signalSensorRadius = 2.0;
  privParams.responsiveness = 0.5;
  privParams.responsivenessCurveKFactor = 2;
  privParams.longProbeDistance = 16;
  privParams.shortProbeBarrierDistance = 4;
  privParams.valenceSaturationMag = 0.5;
  privParams.saveVideo = true;
  privParams.videoStride = 25;
  privParams.videoSaveFirstFrames = 2;
  privParams.displayScale = 8;
  privParams.agentSize = 4;
  privParams.genomeAnalysisStride = privParams.videoStride;
  privParams.displaySampleGenomes = 5;
  privParams.genomeComparisonMethod = 1;
  privParams.updateGraphLog = true;
  privParams.updateGraphLogStride = privParams.videoStride;
  privParams.deterministic = false;
  privParams.RNGSeed = 12345678;
  privParams.graphLogUpdateCommand = "/opt/homebrew/bin/gnuplot --persist ./tools/graphlog.gp";
  privParams.parameterChangeGenerationNumber = 0;
}

/**
 * @brief Registers the configuration file path for later loading
 *
 * @param filename Path to the configuration file (typically config/biosim4.ini)
 *
 * This method stores the filename but does not load the file.
 * Call updateFromConfigFile() to actually parse and apply the config.
 */
void ParamManager::registerConfigFile(const char* filename) {
  configFilename = std::string(filename);
}

/**
 * @brief Checks if a string represents a valid unsigned integer
 *
 * @param s String to validate
 * @return true if string contains only digits (0-9)
 * @return false otherwise
 */
bool checkIfUint(const std::string& s) {
  return s.find_first_not_of("0123456789") == std::string::npos;
}

/**
 * @brief Checks if a string represents a valid signed integer
 *
 * @param s String to validate
 * @return true if string can be parsed as a signed integer with no extra characters
 * @return false if parsing fails or leaves unconsumed characters
 *
 * Uses std::istringstream with noskipws to reject leading whitespace.
 */
bool checkIfInt(const std::string& s) {
  /// return s.find_first_not_of("-0123456789") == std::string::npos;
  std::istringstream iss(s);
  int i;
  iss >> std::noskipws >> i;  ///< noskipws considers leading whitespace invalid
  /// Check the entire string was consumed and if either failbit or badbit is set
  return iss.eof() && !iss.fail();
}

/**
 * @brief Checks if a string represents a valid floating-point number
 *
 * @param s String to validate
 * @return true if string can be parsed as a double with no extra characters
 * @return false if parsing fails or leaves unconsumed characters
 *
 * Uses std::istringstream with noskipws to reject leading whitespace.
 */
bool checkIfFloat(const std::string& s) {
  std::istringstream iss(s);
  double d;
  iss >> std::noskipws >> d;  ///< noskipws considers leading whitespace invalid
  /// Check the entire string was consumed and if either failbit or badbit is set
  return iss.eof() && !iss.fail();
}

/**
 * @brief Checks if a string represents a valid boolean value
 *
 * @param s String to validate
 * @return true if string is "0", "1", "true", or "false" (case-sensitive)
 * @return false otherwise
 */
bool checkIfBool(const std::string& s) {
  return s == "0" || s == "1" || s == "true" || s == "false";
}

/**
 * @brief Converts a string to a boolean value
 *
 * @param s String to convert (should be validated with checkIfBool first)
 * @return true if string is "true" or "1"
 * @return false if string is "false" or "0", or any other value
 */
bool getBoolVal(const std::string& s) {
  if (s == "true" || s == "1")
    return true;
  else if (s == "false" || s == "0")
    return false;
  else
    return false;
}

/**
 * @brief Parses and applies a single parameter name-value pair
 *
 * @param name Parameter name (case-insensitive, converted to lowercase)
 * @param val Parameter value as string (validated and converted to appropriate type)
 *
 * This method:
 * 1. Converts parameter name to lowercase for case-insensitive matching
 * 2. Validates the value type (uint, float, bool) and range
 * 3. Updates the corresponding member in privParams if validation passes
 * 4. Prints error message for invalid parameters
 *
 * @note When adding a new parameter, add its parsing logic here (step 3)
 * @note Each parameter has specific validation rules (range checks, type checks)
 */
void ParamManager::ingestParameter(std::string name, std::string val) {
  std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
  /// std::cout << name << " " << val << '\n' << std::endl;

  // Pre-validate and convert value to all possible types
  bool isUint = checkIfUint(val);
  unsigned uVal = isUint ? (unsigned)std::stol(val.c_str()) : 0;
  bool isFloat = checkIfFloat(val);
  double dVal = isFloat ? std::stod(val.c_str()) : 0.0;
  bool isBool = checkIfBool(val);
  bool bVal = getBoolVal(val);

  // Use do-while(0) pattern to allow early break on successful match
  do {
    if (name == "sizex" && isUint && uVal >= 2 && uVal <= (uint16_t)-1) {
      privParams.gridSize_X = uVal;
      break;
    } else if (name == "sizey" && isUint && uVal >= 2 && uVal <= (uint16_t)-1) {
      privParams.gridSize_Y = uVal;
      break;
    } else if (name == "challenge" && isUint && uVal < (uint16_t)-1) {
      privParams.challenge = uVal;
      break;
    } else if (name == "genomeinitiallengthmin" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
      privParams.genomeInitialLengthMin = uVal;
      break;
    } else if (name == "genomeinitiallengthmax" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
      privParams.genomeInitialLengthMax = uVal;
      break;
    } else if (name == "logdir") {
      privParams.logDir = val;
      break;
    } else if (name == "imagedir") {
      privParams.imageDir = val;
      break;
    } else if (name == "population" && isUint && uVal > 0 && uVal < (uint32_t)-1) {
      privParams.population = uVal;
      break;
    } else if (name == "stepspergeneration" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
      privParams.stepsPerGeneration = uVal;
      break;
    } else if (name == "maxgenerations" && isUint && uVal > 0 && uVal < 0x7fffffff) {
      privParams.maxGenerations = uVal;
      break;
    } else if (name == "barriertype" && isUint && uVal < (uint32_t)-1) {
      privParams.barrierType = uVal;
      break;
    } else if (name == "numthreads" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
      privParams.numThreads = uVal;
      break;
    } else if (name == "signallayers" && isUint && uVal < (uint16_t)-1) {
      privParams.signalLayers = uVal;
      break;
    } else if (name == "genomemaxlength" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
      privParams.genomeMaxLength = uVal;
      break;
    } else if (name == "maxnumberneurons" && isUint && uVal > 0 && uVal < (uint16_t)-1) {
      privParams.maxNumberNeurons = uVal;
      break;
    } else if (name == "pointmutationrate" && isFloat && dVal >= 0.0 && dVal <= 1.0) {
      privParams.pointMutationRate = dVal;
      break;
    } else if (name == "geneinsertiondeletionrate" && isFloat && dVal >= 0.0 && dVal <= 1.0) {
      privParams.geneInsertionDeletionRate = dVal;
      break;
    } else if (name == "deletionratio" && isFloat && dVal >= 0.0 && dVal <= 1.0) {
      privParams.deletionRatio = dVal;
      break;
    } else if (name == "killenable" && isBool) {
      privParams.killEnable = bVal;
      break;
    } else if (name == "sexualreproduction" && isBool) {
      privParams.sexualReproduction = bVal;
      break;
    } else if (name == "chooseparentsbyfitness" && isBool) {
      privParams.chooseParentsByFitness = bVal;
      break;
    } else if (name == "populationsensorradius" && isFloat && dVal > 0.0) {
      privParams.populationSensorRadius = dVal;
      break;
    } else if (name == "signalsensorradius" && isFloat && dVal > 0.0) {
      privParams.signalSensorRadius = dVal;
      break;
    } else if (name == "responsiveness" && isFloat && dVal >= 0.0) {
      privParams.responsiveness = dVal;
      break;
    } else if (name == "responsivenesscurvekfactor" && isUint && uVal >= 1 && uVal <= 20) {
      privParams.responsivenessCurveKFactor = uVal;
      break;
    } else if (name == "longprobedistance" && isUint && uVal > 0) {
      privParams.longProbeDistance = uVal;
      break;
    } else if (name == "shortprobebarrierdistance" && isUint && uVal > 0) {
      privParams.shortProbeBarrierDistance = uVal;
      break;
    } else if (name == "valencesaturationmag" && isFloat && dVal >= 0.0) {
      privParams.valenceSaturationMag = dVal;
      break;
    } else if (name == "savevideo" && isBool) {
      privParams.saveVideo = bVal;
      break;
    } else if (name == "videostride" && isUint && uVal > 0) {
      privParams.videoStride = uVal;
      break;
    } else if (name == "videosavefirstframes" && isUint) {
      privParams.videoSaveFirstFrames = uVal;
      break;
    } else if (name == "displayscale" && isUint && uVal > 0) {
      privParams.displayScale = uVal;
      break;
    } else if (name == "agentsize" && isFloat && dVal > 0.0) {
      privParams.agentSize = dVal;
      break;
    } else if (name == "genomeanalysisstride" && isUint && uVal > 0) {
      privParams.genomeAnalysisStride = uVal;
      break;
    } else if (name == "genomeanalysisstride" && val == "videoStride") {
      privParams.genomeAnalysisStride = privParams.videoStride;
      break;
    } else if (name == "displaysamplegenomes" && isUint) {
      privParams.displaySampleGenomes = uVal;
      break;
    } else if (name == "genomecomparisonmethod" && isUint) {
      privParams.genomeComparisonMethod = uVal;
      break;
    } else if (name == "updategraphlog" && isBool) {
      privParams.updateGraphLog = bVal;
      break;
    } else if (name == "updategraphlogstride" && isUint && uVal > 0) {
      privParams.updateGraphLogStride = uVal;
      break;
    } else if (name == "updategraphlogstride" && val == "videoStride") {
      privParams.updateGraphLogStride = privParams.videoStride;
      break;
    } else if (name == "deterministic" && isBool) {
      privParams.deterministic = bVal;
      break;
    } else if (name == "rngseed" && isUint) {
      privParams.RNGSeed = uVal;
      break;
    } else {
      std::cout << "Invalid param: " << name << " = " << val << std::endl;
    }
  } while (0);
}

/**
 * @brief Loads and applies parameters from the registered config file
 *
 * @param generationNumber Current generation number (for generation-specific parameters)
 *
 * This method:
 * 1. Opens the config file (registered via registerConfigFile())
 * 2. Parses each line, ignoring comments (#) and blank lines
 * 3. Supports generation-specific parameters using `@N` syntax (e.g., "population@100 = 5000")
 * 4. Calls ingestParameter() for each valid parameter line
 * 5. Updates parameterChangeGenerationNumber when a parameter becomes active
 *
 * @note Config file format: "parameterName = value  # optional comment"
 * @note Generation-specific format: "parameterName@generationNum = value"
 * @note Parameters with `@N` syntax only apply when generationNumber >= N
 * @note std::ifstream is RAII - automatically closes when out of scope
 */
void ParamManager::updateFromConfigFile(unsigned generationNumber) {
  /// std::ifstream is RAII, i.e. no need to call close
  std::ifstream cFile(configFilename.c_str());
  if (cFile.is_open()) {
    std::string line;
    while (getline(cFile, line)) {
      // Remove all whitespace for easier parsing
      line.erase(std::remove_if(line.begin(), line.end(), isspace), line.end());
      if (line.empty() || line[0] == '#') {
        continue;  // Skip blank lines and comment lines
      }
      auto delimiterPos = line.find("=");
      auto name = line.substr(0, delimiterPos);

      /// Process the generation specifier if present (e.g., "population@100")
      auto generationDelimiterPos = name.find("@");
      if (generationDelimiterPos < name.size()) {
        // Found generation-specific parameter (e.g., "population@100")
        auto generationSpecifier = name.substr(generationDelimiterPos + 1);
        bool isUint = checkIfUint(generationSpecifier);
        if (!isUint) {
          std::cerr << "Invalid generation specifier: " << name << ".\n";
          continue;
        }
        unsigned activeFromGeneration = (unsigned)std::stol(generationSpecifier);
        if (activeFromGeneration > generationNumber) {
          continue;  ///< This parameter value is not active yet
        } else if (activeFromGeneration == generationNumber) {
          /// Parameter value became active at exactly this generation number
          privParams.parameterChangeGenerationNumber = generationNumber;
        }
        // Remove the @N suffix to get the base parameter name
        name = name.substr(0, generationDelimiterPos);
      }

      std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c) { return std::tolower(c); });
      auto value0 = line.substr(delimiterPos + 1);
      auto delimiterComment = value0.find("#");
      auto value = value0.substr(0, delimiterComment);  // Strip inline comments
      auto rawValue = value;                            // Preserve raw value (currently unused)
      value.erase(std::remove_if(value.begin(), value.end(), isspace), value.end());
      /// std::cout << name << " " << value << '\n' << std::endl;
      ingestParameter(name, value);
    }
  } else {
    std::cerr << "Couldn't open config file " << configFilename << ".\n" << std::endl;
  }
}

/**
 * @brief Validates parameter values for consistency and reasonableness
 *
 * Performs cross-parameter validation checks that cannot be done in
 * ingestParameter() (which validates individual parameters in isolation).
 *
 * Currently checks:
 * - Warns if deterministic=true but numThreads != 1 (threading breaks determinism)
 *
 * @note This is typically called once after initial parameter loading
 * @note Add new validation rules here for parameters that depend on each other
 */
void ParamManager::checkParameters() {
  if (privParams.deterministic && privParams.numThreads != 1) {
    std::cerr << "Warning: When deterministic is true, you probably want to set numThreads = 1." << std::endl;
  }
}

}  // namespace BioSim
