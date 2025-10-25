#ifndef LOGGER_H
#define LOGGER_H

// Use spdlog's bundled fmt (no separate fmt dependency needed)
#include <spdlog/fmt/bundled/color.h>
#include <spdlog/fmt/fmt.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <string_view>

namespace BioSim {

/**
 * @brief Centralized logging utility with dual output strategy
 *
 * Architecture:
 * - **Console output**: Direct fmt formatting with colors (immediate, human-friendly)
 * - **File logging**: spdlog with rotation (persistent, structured, filterable)
 *
 * Usage:
 * @code
 * // Initialize at program start (in main.cpp)
 * Logger::init("output/logs/biosim4.log");
 *
 * // Console output (colored, no file logging)
 * Logger::print("Starting simulation with {} generations", params.maxGenerations);
 * Logger::success("Video saved to {}", filepath);
 * Logger::warning("Invalid parameter: {}", key);
 * Logger::error("Failed to load config: {}", e.what());
 *
 * // File logging (persistent, with levels)
 * Logger::info("Generation {} complete, {} survivors", gen, count);
 * Logger::debug("Genome length: min={} max={} avg={}", min, max, avg);
 * @endcode
 *
 * Log levels (file only):
 * - TRACE: Extremely verbose debugging
 * - DEBUG: Detailed diagnostic information
 * - INFO: General informational messages
 * - WARN: Warning conditions
 * - ERROR: Error conditions
 * - CRITICAL: Critical failures
 */
class Logger {
 public:
  /**
   * @brief Initialize the logging system
   * @param logPath Path to log file (will create rotating logs with 5MB max, 3 files)
   * @param level Minimum log level (default: info)
   */
  static void init(const std::string& logPath = "output/logs/biosim4.log",
                   spdlog::level::level_enum level = spdlog::level::info) {
    try {
      // Create rotating file logger: 5MB max size, 3 backup files
      auto file_logger = spdlog::rotating_logger_mt("biosim", logPath, 1024 * 1024 * 5, 3);
      file_logger->set_level(level);
      file_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");
      spdlog::set_default_logger(file_logger);

      file_logger->info("========================================");
      file_logger->info("BioSim4 logging initialized");
      file_logger->info("========================================");
    } catch (const spdlog::spdlog_ex& ex) {
      fmt::print(stderr, fg(fmt::color::red), "Failed to initialize logger: {}\n", ex.what());
    }
  }

  /**
   * @brief Shutdown logging system (flush and close files)
   */
  static void shutdown() {
    if (spdlog::get("biosim")) {
      spdlog::get("biosim")->info("BioSim4 logging shutdown");
    }
    spdlog::shutdown();
  }

  // ============================================================================
  // Console Output (fmt-based, colored, no file logging)
  // ============================================================================

  /**
   * @brief Print to console only (no log file)
   */
  template <typename... Args>
  static void print(const char* fmt_str, Args&&... args) {
    fmt::print(fmt::runtime(fmt_str), std::forward<Args>(args)...);
    fmt::print("\n");
  }

  /**
   * @brief Print success message (green checkmark, console only)
   */
  template <typename... Args>
  static void success(fmt::format_string<Args...> fmt_str, Args&&... args) {
    fmt::print(fmt::fg(fmt::color::green), "✓ ");
    fmt::print(fmt::fg(fmt::color::green), fmt_str, std::forward<Args>(args)...);
    fmt::print("\n");
  }

  /**
   * @brief Print warning (yellow, console only)
   */
  template <typename... Args>
  static void warning(fmt::format_string<Args...> fmt_str, Args&&... args) {
    fmt::print(fmt::fg(fmt::color::yellow) | fmt::emphasis::bold, "⚠  ");
    fmt::print(fmt::fg(fmt::color::yellow), fmt_str, std::forward<Args>(args)...);
    fmt::print("\n");
  }

  /**
   * @brief Print error (red/bold, console only)
   */
  template <typename... Args>
  static void error(fmt::format_string<Args...> fmt_str, Args&&... args) {
    fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, "✗ ");
    fmt::print(stderr, fmt::fg(fmt::color::red) | fmt::emphasis::bold, fmt_str, std::forward<Args>(args)...);
    fmt::print(stderr, "\n");
  }

  /**
   * @brief Print header/banner (cyan/bold, console only)
   */
  template <typename... Args>
  static void header(fmt::format_string<Args...> fmt_str, Args&&... args) {
    fmt::print(fmt::fg(fmt::color::cyan) | fmt::emphasis::bold, fmt_str, std::forward<Args>(args)...);
    fmt::print("\n");
  }

  // ============================================================================
  // File Logging (spdlog-based, persistent, structured)
  // ============================================================================

  /**
   * @brief Log informational message to file
   */
  template <typename... Args>
  static void info(fmt::format_string<Args...> fmt_str, Args&&... args) {
    if (auto logger = spdlog::get("biosim")) {
      logger->info(fmt_str, std::forward<Args>(args)...);
    }
  }

  /**
   * @brief Log debug message to file
   */
  template <typename... Args>
  static void debug(fmt::format_string<Args...> fmt_str, Args&&... args) {
    if (auto logger = spdlog::get("biosim")) {
      logger->debug(fmt_str, std::forward<Args>(args)...);
    }
  }

  /**
   * @brief Log trace message to file (very verbose)
   */
  template <typename... Args>
  static void trace(fmt::format_string<Args...> fmt_str, Args&&... args) {
    if (auto logger = spdlog::get("biosim")) {
      logger->trace(fmt_str, std::forward<Args>(args)...);
    }
  }

  /**
   * @brief Log warning to file
   */
  template <typename... Args>
  static void warn(fmt::format_string<Args...> fmt_str, Args&&... args) {
    if (auto logger = spdlog::get("biosim")) {
      logger->warn(fmt_str, std::forward<Args>(args)...);
    }
  }

  /**
   * @brief Log error to file
   */
  template <typename... Args>
  static void log_error(fmt::format_string<Args...> fmt_str, Args&&... args) {
    if (auto logger = spdlog::get("biosim")) {
      logger->error(fmt_str, std::forward<Args>(args)...);
    }
  }

  /**
   * @brief Log critical error to file
   */
  template <typename... Args>
  static void critical(fmt::format_string<Args...> fmt_str, Args&&... args) {
    if (auto logger = spdlog::get("biosim")) {
      logger->critical(fmt_str, std::forward<Args>(args)...);
    }
  }

  /**
   * @brief Flush log buffers to disk
   */
  static void flush() {
    if (auto logger = spdlog::get("biosim")) {
      logger->flush();
    }
  }

  /**
   * @brief Set minimum log level
   */
  static void setLevel(spdlog::level::level_enum level) {
    if (auto logger = spdlog::get("biosim")) {
      logger->set_level(level);
    }
  }
};

}  // namespace BioSim

#endif  // LOGGER_H
