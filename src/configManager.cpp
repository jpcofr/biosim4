/**
 * @file configManager.cpp
 * @brief Implementation of modern TOML-based configuration system
 */

#include "configManager.h"

#include "logger.h"

#include <spdlog/fmt/fmt.h>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <toml.hpp>

namespace BioSim {

ConfigManager::ConfigManager() {
  // Initialize with built-in defaults (from legacy ParamManager)
  params_ = Params{};  // Uses default initialization

  // Set default directory paths (must be done explicitly since Params{} zero-initializes)
  params_.logDir = "./output/logs/";
  params_.imageDir = "./output/images/";
  params_.graphLogUpdateCommand = "/opt/homebrew/bin/gnuplot --persist ./tools/graphlog.gp";

  // Set other critical defaults from ParamManager::setDefaults()
  params_.gridSize_X = 128;
  params_.gridSize_Y = 128;
  params_.challenge = 6;
  params_.genomeInitialLengthMin = 24;
  params_.genomeInitialLengthMax = 24;
  params_.genomeMaxLength = 300;
  params_.population = 3000;
  params_.stepsPerGeneration = 300;
  params_.maxGenerations = 200000;
  params_.barrierType = 0;
  params_.numThreads = 4;
  params_.signalLayers = 1;
  params_.maxNumberNeurons = 5;
  params_.pointMutationRate = 0.001;
  params_.geneInsertionDeletionRate = 0.0;
  params_.deletionRatio = 0.5;
  params_.killEnable = false;
  params_.sexualReproduction = true;
  params_.chooseParentsByFitness = true;
  params_.populationSensorRadius = 2.5;
  params_.signalSensorRadius = 2.0;
  params_.responsiveness = 0.5;
  params_.responsivenessCurveKFactor = 2;
  params_.longProbeDistance = 16;
  params_.shortProbeBarrierDistance = 4;
  params_.valenceSaturationMag = 0.5;
  params_.saveVideo = true;
  params_.videoStride = 25;
  params_.videoSaveFirstFrames = 2;
  params_.displayScale = 8;
  params_.agentSize = 4;
  params_.genomeAnalysisStride = params_.videoStride;
  params_.displaySampleGenomes = 5;
  params_.genomeComparisonMethod = 1;
  params_.updateGraphLog = true;
  params_.updateGraphLogStride = params_.videoStride;
  params_.deterministic = false;
  params_.RNGSeed = 12345678;
  params_.parameterChangeGenerationNumber = 0;

  initializePresets();
}

bool ConfigManager::load(const std::string& configPath, const std::map<std::string, std::string>& overrides) {
  // Step 1: Load from file (if specified or found)
  if (!configPath.empty()) {
    if (!std::filesystem::exists(configPath)) {
      Logger::error("Config file not found: {}", configPath);
      return false;
    }
    if (!loadFromToml(configPath)) {
      return false;
    }
  } else {
    // Auto-search for config file
    auto foundPath = findConfigFile();
    if (foundPath) {
      Logger::print("📄 Found config: {}", foundPath->string());
      if (!loadFromToml(*foundPath)) {
        return false;
      }
    } else {
      Logger::print("ℹ️  No config file found, using defaults");
    }
  }

  // Step 2: Apply environment variable overrides
  applyEnvironmentOverrides();

  // Step 3: Apply command-line overrides
  for (const auto& [key, value] : overrides) {
    if (!setParameter(key, value)) {
      Logger::warning("Failed to apply override: {}={}", key, value);
    } else {
      Logger::print("⚙️  Override: {} = {}", key, value);
    }
  }

  // Step 4: Validate final configuration
  try {
    validate();
  } catch (const std::exception& e) {
    Logger::error("Configuration validation failed: {}", e.what());
    return false;
  }

  return true;
}

std::optional<std::filesystem::path> ConfigManager::findConfigFile() const {
  const std::vector<std::filesystem::path> searchPaths = {
      "biosim4.toml",         // Current directory
      "config/biosim4.toml",  // Standard location
  };

  for (const auto& path : searchPaths) {
    if (std::filesystem::exists(path)) {
      return path;
    }
  }

  return std::nullopt;
}

bool ConfigManager::loadFromToml(const std::filesystem::path& path) {
  try {
    const auto data = toml::parse(path);
    loadedConfigPath_ = path.string();

    // Load parameters from TOML sections
    // [simulation] section
    if (data.contains("simulation")) {
      const auto& sim = toml::find(data, "simulation");
      if (sim.contains("sizeX"))
        params_.gridSize_X = toml::find<int>(sim, "sizeX");
      if (sim.contains("sizeY"))
        params_.gridSize_Y = toml::find<int>(sim, "sizeY");
      if (sim.contains("population"))
        params_.population = toml::find<int>(sim, "population");
      if (sim.contains("stepsPerGeneration"))
        params_.stepsPerGeneration = toml::find<int>(sim, "stepsPerGeneration");
      if (sim.contains("maxGenerations"))
        params_.maxGenerations = toml::find<int>(sim, "maxGenerations");
    }

    // [genome] section
    if (data.contains("genome")) {
      const auto& gen = toml::find(data, "genome");
      if (gen.contains("genomeInitialLengthMin"))
        params_.genomeInitialLengthMin = toml::find<int>(gen, "genomeInitialLengthMin");
      if (gen.contains("genomeInitialLengthMax"))
        params_.genomeInitialLengthMax = toml::find<int>(gen, "genomeInitialLengthMax");
      if (gen.contains("genomeMaxLength"))
        params_.genomeMaxLength = toml::find<int>(gen, "genomeMaxLength");
      if (gen.contains("maxNumberNeurons"))
        params_.maxNumberNeurons = toml::find<int>(gen, "maxNumberNeurons");
    }

    // [video] section
    if (data.contains("video")) {
      const auto& vid = toml::find(data, "video");
      if (vid.contains("saveVideo"))
        params_.saveVideo = toml::find<bool>(vid, "saveVideo");
      if (vid.contains("videoStride"))
        params_.videoStride = toml::find<int>(vid, "videoStride");
      if (vid.contains("videoSaveFirstFrames"))
        params_.videoSaveFirstFrames = toml::find<int>(vid, "videoSaveFirstFrames");
      if (vid.contains("displayScale"))
        params_.displayScale = toml::find<int>(vid, "displayScale");
    }

    // [performance] section
    if (data.contains("performance")) {
      const auto& perf = toml::find(data, "performance");
      if (perf.contains("numThreads"))
        params_.numThreads = toml::find<int>(perf, "numThreads");
    }

    // [challenge] section
    if (data.contains("challenge")) {
      const auto& chal = toml::find(data, "challenge");
      if (chal.contains("challenge"))
        params_.challenge = toml::find<int>(chal, "challenge");
    }

    Logger::success("Loaded config from {}", path.string());
    return true;

  } catch (const std::exception& e) {
    Logger::error("Failed to parse TOML: {}", e.what());
    return false;
  }
}

void ConfigManager::initializePresets() {
  // Quick test preset - fast execution for development
  presets_["quick"] = ConfigPreset{"quick", "Fast test: 10 generations, small population, no video", [](Params& p) {
                                     p.maxGenerations = 10;
                                     p.population = 100;
                                     p.stepsPerGeneration = 50;
                                     p.saveVideo = false;
                                     p.numThreads = 1;
                                   }};

  // Video test preset - verify video generation works
  presets_["video-test"] =
      ConfigPreset{"video-test", "Video generation test: 5 generations, all frames saved", [](Params& p) {
                     p.maxGenerations = 5;
                     p.population = 200;
                     p.stepsPerGeneration = 100;
                     p.saveVideo = true;
                     p.videoStride = 1;  // Save every generation
                     p.videoSaveFirstFrames = 5;
                     p.displayScale = 4;
                     p.numThreads = 1;
                   }};

  // Micro test preset - from legacy testapp.py
  presets_["microtest"] =
      ConfigPreset{"microtest", "Minimal test: 11 generations, tiny genome, single-threaded", [](Params& p) {
                     p.maxGenerations = 11;
                     p.population = 100;
                     p.genomeInitialLengthMin = 20;
                     p.genomeInitialLengthMax = 20;
                     p.genomeMaxLength = 30;
                     p.maxNumberNeurons = 2;
                     p.numThreads = 1;
                     p.saveVideo = false;
                   }};

  // Benchmark preset - performance testing
  presets_["benchmark"] =
      ConfigPreset{"benchmark", "Performance benchmark: Large population, multi-threaded", [](Params& p) {
                     p.maxGenerations = 100;
                     p.population = 5000;
                     p.stepsPerGeneration = 300;
                     p.saveVideo = false;
                     p.numThreads = 0;  // Use all available cores
                   }};

  // Demo preset - visually interesting for demonstrations
  presets_["demo"] =
      ConfigPreset{"demo", "Demonstration: Moderate run with video, nice for showing off", [](Params& p) {
                     p.maxGenerations = 50;
                     p.population = 1000;
                     p.stepsPerGeneration = 200;
                     p.saveVideo = true;
                     p.videoStride = 5;
                     p.videoSaveFirstFrames = 3;
                     p.displayScale = 6;
                   }};
}

bool ConfigManager::applyPreset(const std::string& presetName) {
  auto it = presets_.find(presetName);
  if (it == presets_.end()) {
    Logger::error("Unknown preset: {}", presetName);
    fmt::print(stderr, "Available presets: ");
    bool first = true;
    for (const auto& [name, _] : presets_) {
      if (!first)
        fmt::print(stderr, ", ");
      fmt::print(stderr, "{}", name);
      first = false;
    }
    fmt::print(stderr, "\n");
    return false;
  }

  it->second.apply(params_);
  Logger::success("Applied preset: {} - {}", presetName, it->second.description);
  return true;
}

std::vector<std::string> ConfigManager::getAvailablePresets() const {
  std::vector<std::string> names;
  for (const auto& [name, _] : presets_) {
    names.push_back(name);
  }
  std::sort(names.begin(), names.end());
  return names;
}

std::string ConfigManager::getPresetDescription(const std::string& presetName) const {
  auto it = presets_.find(presetName);
  return (it != presets_.end()) ? it->second.description : "";
}

bool ConfigManager::setParameter(const std::string& key, const std::string& value) {
  return setParameterInternal(key, value);
}

bool ConfigManager::setParameterInternal(const std::string& key, const std::string& value) {
  try {
    // Simulation parameters
    if (key == "sizeX") {
      params_.gridSize_X = std::stoi(value);
    } else if (key == "sizeY") {
      params_.gridSize_Y = std::stoi(value);
    } else if (key == "population") {
      params_.population = std::stoi(value);
    } else if (key == "stepsPerGeneration") {
      params_.stepsPerGeneration = std::stoi(value);
    } else if (key == "maxGenerations") {
      params_.maxGenerations = std::stoi(value);
    }
    // Genome parameters
    else if (key == "genomeInitialLengthMin") {
      params_.genomeInitialLengthMin = std::stoi(value);
    } else if (key == "genomeInitialLengthMax") {
      params_.genomeInitialLengthMax = std::stoi(value);
    } else if (key == "genomeMaxLength") {
      params_.genomeMaxLength = std::stoi(value);
    } else if (key == "maxNumberNeurons") {
      params_.maxNumberNeurons = std::stoi(value);
    }
    // Video parameters
    else if (key == "saveVideo") {
      std::string v = value;
      std::transform(v.begin(), v.end(), v.begin(), ::tolower);
      params_.saveVideo = (v == "true" || v == "1" || v == "yes");
    } else if (key == "videoStride") {
      params_.videoStride = std::stoi(value);
    } else if (key == "videoSaveFirstFrames") {
      params_.videoSaveFirstFrames = std::stoi(value);
    } else if (key == "displayScale") {
      params_.displayScale = std::stoi(value);
    }
    // Performance parameters
    else if (key == "numThreads") {
      params_.numThreads = std::stoi(value);
    }
    // Challenge parameter
    else if (key == "challenge") {
      params_.challenge = std::stoi(value);
    } else {
      Logger::warning("Unknown parameter: {}", key);
      return false;
    }
    return true;
  } catch (const std::exception& e) {
    Logger::error("Invalid value for {}: {}", key, value);
    return false;
  }
}

void ConfigManager::validate() const {
  // Grid size validation
  if (params_.gridSize_X < 16 || params_.gridSize_X > 2048) {
    throw std::invalid_argument("sizeX must be 16-2048, got " + std::to_string(params_.gridSize_X));
  }
  if (params_.gridSize_Y < 16 || params_.gridSize_Y > 2048) {
    throw std::invalid_argument("sizeY must be 16-2048, got " + std::to_string(params_.gridSize_Y));
  }

  // Population validation
  if (params_.population < 1 || params_.population > 100000) {
    throw std::invalid_argument("population must be 1-100000, got " + std::to_string(params_.population));
  }

  // Check population fits in grid
  int maxPopulation = (params_.gridSize_X * params_.gridSize_Y) / 4;  // 25% density max
  if (params_.population > maxPopulation) {
    throw std::invalid_argument("population (" + std::to_string(params_.population) + ") too large for grid " +
                                std::to_string(params_.gridSize_X) + "x" + std::to_string(params_.gridSize_Y) +
                                " (max ~" + std::to_string(maxPopulation) + ")");
  }

  // Generation parameters
  if (params_.stepsPerGeneration < 1 || params_.stepsPerGeneration > 10000) {
    throw std::invalid_argument("stepsPerGeneration must be 1-10000, got " +
                                std::to_string(params_.stepsPerGeneration));
  }
  if (params_.maxGenerations < 1) {
    throw std::invalid_argument("maxGenerations must be >= 1, got " + std::to_string(params_.maxGenerations));
  }

  // Genome validation
  if (params_.genomeInitialLengthMin < 1) {
    throw std::invalid_argument("genomeInitialLengthMin must be >= 1");
  }
  if (params_.genomeInitialLengthMax < params_.genomeInitialLengthMin) {
    throw std::invalid_argument("genomeInitialLengthMax must be >= genomeInitialLengthMin");
  }
  if (params_.genomeMaxLength < params_.genomeInitialLengthMax) {
    throw std::invalid_argument("genomeMaxLength must be >= genomeInitialLengthMax");
  }

  // Video validation
  if (params_.displayScale < 1 || params_.displayScale > 32) {
    throw std::invalid_argument("displayScale must be 1-32, got " + std::to_string(params_.displayScale));
  }
}

void ConfigManager::applyEnvironmentOverrides() {
  // Check for environment variables with BIOSIM_ prefix
  const char* envVars[] = {
      "BIOSIM_POPULATION",    "BIOSIM_GENERATIONS", "BIOSIM_SAVE_VIDEO", "BIOSIM_NUM_THREADS",   "BIOSIM_VIDEO_STRIDE",
      "BIOSIM_DISPLAY_SCALE", "BIOSIM_SIZE_X",      "BIOSIM_SIZE_Y",     "BIOSIM_STEPS_PER_GEN", nullptr};

  for (int i = 0; envVars[i] != nullptr; ++i) {
    const char* value = std::getenv(envVars[i]);
    if (value != nullptr) {
      std::string key = envVars[i];
      // Remove BIOSIM_ prefix and convert to camelCase
      key = key.substr(7);  // Remove "BIOSIM_"
      std::transform(key.begin(), key.end(), key.begin(), ::tolower);

      // Convert underscores to camelCase
      std::string camelKey;
      bool nextUpper = false;
      for (char c : key) {
        if (c == '_') {
          nextUpper = true;
        } else if (nextUpper) {
          camelKey += std::toupper(c);
          nextUpper = false;
        } else {
          camelKey += c;
        }
      }

      if (setParameter(camelKey, value)) {
        Logger::print("🌍 Environment override: {}={}", envVars[i], value);
      }
    }
  }
}

void ConfigManager::exportToFile(const std::string& path) const {
  std::ofstream file(path);
  if (!file) {
    throw std::runtime_error("Failed to open file for writing: " + path);
  }

  file << "# BioSim4 Configuration File (TOML format)\n";
  file << "# Generated by ConfigManager\n\n";

  file << "[simulation]\n";
  file << "sizeX = " << params_.gridSize_X << "\n";
  file << "sizeY = " << params_.gridSize_Y << "\n";
  file << "population = " << params_.population << "\n";
  file << "stepsPerGeneration = " << params_.stepsPerGeneration << "\n";
  file << "maxGenerations = " << params_.maxGenerations << "\n\n";

  file << "[genome]\n";
  file << "genomeInitialLengthMin = " << params_.genomeInitialLengthMin << "\n";
  file << "genomeInitialLengthMax = " << params_.genomeInitialLengthMax << "\n";
  file << "genomeMaxLength = " << params_.genomeMaxLength << "\n";
  file << "maxNumberNeurons = " << params_.maxNumberNeurons << "\n\n";

  file << "[video]\n";
  file << "saveVideo = " << (params_.saveVideo ? "true" : "false") << "\n";
  file << "videoStride = " << params_.videoStride << "\n";
  file << "videoSaveFirstFrames = " << params_.videoSaveFirstFrames << "\n";
  file << "displayScale = " << params_.displayScale << "\n\n";

  file << "[performance]\n";
  file << "numThreads = " << params_.numThreads << "\n\n";

  file << "[challenge]\n";
  file << "challenge = " << params_.challenge << "\n";

  Logger::success("Configuration exported to {}", path);
}

void ConfigManager::printConfig(bool showDefaults) const {
  fmt::print("\n╔══════════════════════════════════════════╗\n");
  fmt::print("║       Current Configuration              ║\n");
  fmt::print("╚══════════════════════════════════════════╝\n\n");

  fmt::print("Simulation:\n");
  fmt::print("  Grid: {} × {}\n", params_.gridSize_X, params_.gridSize_Y);
  fmt::print("  Population: {}\n", params_.population);
  fmt::print("  Generations: {}\n", params_.maxGenerations);
  fmt::print("  Steps/Gen: {}\n\n", params_.stepsPerGeneration);

  fmt::print("Genome:\n");
  fmt::print("  Initial length: {}-{}\n", params_.genomeInitialLengthMin, params_.genomeInitialLengthMax);
  fmt::print("  Max length: {}\n", params_.genomeMaxLength);
  fmt::print("  Max neurons: {}\n\n", params_.maxNumberNeurons);

  fmt::print("Video:\n");
  fmt::print("  Save video: {}\n", params_.saveVideo ? "Yes" : "No");
  if (params_.saveVideo) {
    fmt::print("  Video stride: {}\n", params_.videoStride);
    fmt::print("  Save first: {} frames\n", params_.videoSaveFirstFrames);
    fmt::print("  Display scale: {}x\n", params_.displayScale);
  }
  fmt::print("\n");

  fmt::print("Performance:\n");
  fmt::print("  Threads: {}\n\n", params_.numThreads == 0 ? "auto" : std::to_string(params_.numThreads));

  if (loadedConfigPath_) {
    fmt::print("📄 Loaded from: {}\n", *loadedConfigPath_);
  } else {
    fmt::print("📄 Using default configuration\n");
  }
  fmt::print("\n");
}

}  // namespace BioSim
