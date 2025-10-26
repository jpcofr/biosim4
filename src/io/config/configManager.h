/**
 * @file configManager.h
 * @brief Modern configuration management with TOML support and presets
 *
 * Replaces the legacy INI-based ParamManager with a flexible system that supports:
 * - TOML configuration files (more expressive than INI)
 * - Code-based configuration presets
 * - Command-line overrides
 * - Validation with clear error messages
 * - Easy testing without file creation
 */

#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include "../../types/params.h"

#include <filesystem>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace BioSim {
inline namespace v1 {
namespace IO {
namespace Config {

/**
 * @brief Configuration preset definition
 *
 * Presets allow quick configuration changes without editing files.
 * Examples: "quick" for fast testing, "video-test" for video validation.
 */
struct ConfigPreset {
  std::string name;
  std::string description;
  std::function<void(Params&)> apply;
};

/**
 * @brief Modern configuration manager with TOML support
 *
 * This class replaces the legacy ParamManager with a more maintainable
 * and flexible configuration system. Key improvements:
 * - TOML format (human-friendly, well-specified)
 * - Built-in presets for common scenarios
 * - Hierarchical config search
 * - Command-line overrides
 * - Comprehensive validation
 *
 * Priority order (highest to lowest):
 * 1. Command-line overrides (--set key=value)
 * 2. Environment variables (BIOSIM_*)
 * 3. Specified config file
 * 4. Standard locations (./biosim4.toml, config/biosim4.toml)
 * 5. Built-in defaults
 */
class ConfigManager {
 public:
  ConfigManager();

  /**
   * @brief Load configuration from file with optional overrides
   *
   * @param configPath Path to TOML config file (empty = auto-search)
   * @param overrides Map of key-value pairs to override after loading
   * @return true if config loaded successfully
   */
  bool load(const std::string& configPath = "", const std::map<std::string, std::string>& overrides = {});

  /**
   * @brief Apply a named preset configuration
   *
   * @param presetName Name of preset (e.g., "quick", "video-test")
   * @return true if preset exists and was applied
   */
  bool applyPreset(const std::string& presetName);

  /**
   * @brief Get list of available presets
   */
  std::vector<std::string> getAvailablePresets() const;

  /**
   * @brief Get preset description
   */
  std::string getPresetDescription(const std::string& presetName) const;

  /**
   * @brief Set a parameter value with validation
   *
   * @param key Parameter name (e.g., "population", "sizeX")
   * @param value String representation of value
   * @return true if parameter was set successfully
   */
  bool setParameter(const std::string& key, const std::string& value);

  /**
   * @brief Validate current configuration
   *
   * @throws std::invalid_argument if configuration is invalid
   */
  void validate() const;

  /**
   * @brief Get immutable reference to current parameters
   */
  const Params& getParams() const { return params_; }

  /**
   * @brief Export current configuration to TOML file
   *
   * Useful for saving a preset as a persistent config file.
   *
   * @param path Output file path
   */
  void exportToFile(const std::string& path) const;

  /**
   * @brief Print current configuration to stdout
   *
   * @param showDefaults If true, show all params. If false, only non-defaults.
   */
  void printConfig(bool showDefaults = false) const;

  /**
   * @brief Get path to config file that was loaded (if any)
   */
  std::optional<std::string> getLoadedConfigPath() const { return loadedConfigPath_; }

 private:
  Params params_;
  std::optional<std::string> loadedConfigPath_;
  std::map<std::string, ConfigPreset> presets_;

  /**
   * @brief Search for config file in standard locations
   */
  std::optional<std::filesystem::path> findConfigFile() const;

  /**
   * @brief Load TOML file and populate params
   */
  bool loadFromToml(const std::filesystem::path& path);

  /**
   * @brief Initialize built-in presets
   */
  void initializePresets();

  /**
   * @brief Apply environment variable overrides
   */
  void applyEnvironmentOverrides();

  /**
   * @brief Convert string value to appropriate type and set parameter
   */
  bool setParameterInternal(const std::string& key, const std::string& value);
};

}  // namespace Config
}  // namespace IO
}  // namespace v1
}  // namespace BioSim

// Backward compatibility aliases
namespace BioSim {
using IO::Config::ConfigManager;
using IO::Config::ConfigPreset;
}  // namespace BioSim

#endif  // CONFIGMANAGER_H
