/**
 * @file main.cpp
 * @brief Modern CLI entry point for BioSim4
 *
 * Features:
 * - TOML-based configuration (backwards compatible with INI)
 * - Built-in presets for common scenarios
 * - Command-line overrides
 * - Interactive video verification
 * - Helpful error messages
 */

#include "CLI/CLI.hpp"
#include "configManager.h"
#include "logger.h"
#include "videoVerifier.h"

#include <spdlog/fmt/fmt.h>

#include <map>
#include <string>

namespace BioSim {
inline namespace v1 {
namespace Core {
namespace Simulation {
void simulator(const Types::Params& params);
}
}  // namespace Core
}  // namespace v1
}  // namespace BioSim

/**
 * @brief Display available configuration presets
 */
void showPresets(BioSim::ConfigManager& config) {
  fmt::print("\n📋 Available Presets:\n\n");
  for (const auto& preset : config.getAvailablePresets()) {
    fmt::print("  • {}\n", preset);
    fmt::print("    {}\n\n", config.getPresetDescription(preset));
  }
}

/**
 * @brief Main program entry point
 */
int main(int argc, char** argv) {
  CLI::App app{"BioSim4 - Biological Evolution Simulator"};
  app.footer(
      "\nExamples:\n"
      "  biosim4                           # Use default config\n"
      "  biosim4 --preset quick            # Quick test run\n"
      "  biosim4 --preset video-test       # Test video generation\n"
      "  biosim4 config.toml               # Use specific config\n"
      "  biosim4 --set population=500      # Override parameter\n"
      "  biosim4 --verify-videos           # Check generated videos\n");

  // Configuration options
  std::string configFile;
  app.add_option("-c,--config", configFile, "Config file path (TOML)")->check(CLI::ExistingFile);

  std::string preset;
  app.add_option("-p,--preset", preset, "Use configuration preset");

  std::vector<std::string> overrides;
  app.add_option("-s,--set", overrides, "Override parameters (e.g., population=500)")->expected(0, -1);

  bool listPresets = false;
  app.add_flag("-l,--list-presets", listPresets, "List available presets");

  bool showConfig = false;
  app.add_flag("--show-config", showConfig, "Print configuration and exit");

  [[maybe_unused]] bool exportConfig = false;
  std::string exportPath;
  app.add_option("--export-config", exportPath, "Export current config to file and exit");

  // Video verification options
  bool verifyVideos = false;
  app.add_flag("--verify-videos", verifyVideos, "Verify generated videos and exit");

  bool interactiveVideos = false;
  app.add_flag("--review-videos", interactiveVideos, "Interactive video review mode");

  std::string videoDir = "output/images";
  app.add_option("--video-dir", videoDir, "Directory containing videos")->default_val("output/images");

  // Parse command line
  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

  // Create configuration manager
  BioSim::ConfigManager config;

  // Handle --list-presets
  if (listPresets) {
    showPresets(config);
    return 0;
  }

  // Handle video verification mode
  if (verifyVideos) {
    BioSim::Logger::print("🔍 Verifying video generation...");
    auto result = BioSim::VideoVerifier::verify(videoDir, 5, true);  // Default: check 5 videos
    return result.success ? 0 : 1;
  }

  // Handle interactive video review
  if (interactiveVideos) {
    BioSim::VideoVerifier::interactiveReview(videoDir);
    return 0;
  }

  // Load configuration
  std::map<std::string, std::string> overrideMap;
  for (const auto& override : overrides) {
    size_t pos = override.find('=');
    if (pos != std::string::npos) {
      overrideMap[override.substr(0, pos)] = override.substr(pos + 1);
    } else {
      BioSim::Logger::warning("Invalid override format: {} (expected key=value)", override);
    }
  }

  if (!config.load(configFile, overrideMap)) {
    BioSim::Logger::error("Failed to load configuration");
    return 1;
  }

  // Apply preset if specified
  if (!preset.empty()) {
    if (!config.applyPreset(preset)) {
      return 1;
    }
  }

  // Handle --show-config
  if (showConfig) {
    config.printConfig(true);
    return 0;
  }

  // Handle --export-config
  if (!exportPath.empty()) {
    try {
      config.exportToFile(exportPath);
      return 0;
    } catch (const std::exception& e) {
      BioSim::Logger::error("Export failed: {}", e.what());
      return 1;
    }
  }

  // Initialize logging system
  const auto& params = config.getParams();
  BioSim::Logger::init(params.logDir + "/biosim4.log", spdlog::level::info);
  BioSim::Logger::info("=== BioSim4 Session Start ===");
  BioSim::Logger::info("Configuration: grid={}x{}, population={}, generations={}", params.gridSize_X, params.gridSize_Y,
                       params.population, params.maxGenerations);

  // Print configuration summary to console
  BioSim::Logger::header("\n🧬 BioSim4 Starting...");
  config.printConfig(false);

  // Run simulation
  try {
    BioSim::Core::Simulation::simulator(config.getParams());
  } catch (const std::exception& e) {
    BioSim::Logger::error("\nSimulation failed: {}", e.what());
    BioSim::Logger::log_error("Simulation failed with exception: {}", e.what());
    BioSim::Logger::shutdown();
    return 1;
  }

  // Post-simulation: Offer video verification
  if (params.saveVideo) {
    fmt::print("\n🎬 Videos saved to {}/\n", videoDir);
    fmt::print("\nTo verify videos, run:\n");
    fmt::print("  ./biosim4 --verify-videos\n");
    fmt::print("  ./biosim4 --review-videos\n");
  }

  BioSim::Logger::success("\nSimulation complete!");
  BioSim::Logger::info("=== BioSim4 Session End ===");
  BioSim::Logger::shutdown();
  return 0;
}
