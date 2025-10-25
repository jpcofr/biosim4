#ifndef BIOSIM4_INCLUDE_RANDOM_H_
#define BIOSIM4_INCLUDE_RANDOM_H_

/**
 * @file random.h
 * @brief Fast random number generation for simulation
 *
 * Provides thread-safe random number generation using a hybrid approach:
 * - Marsaglia algorithm for base generation
 * - Jenkins algorithm for additional mixing
 *
 * See random.cpp for implementation details and algorithm notes.
 */

#include <climits>
#include <cstdint>

namespace BioSim {

/**
 * @struct RandomUintGenerator
 * @brief Thread-safe pseudo-random number generator
 *
 * Implements a fast PRNG suitable for simulation use. Each thread maintains
 * its own instance (via OpenMP threadprivate pragma) to avoid contention.
 *
 * ## Algorithms
 * - **Marsaglia**: Base RNG with good statistical properties
 * - **Jenkins**: Additional mixing for improved randomness
 *
 * ## Usage
 * ```cpp
 * randomUint.initialize();           // Seed the generator
 * uint32_t r1 = randomUint();        // Get random uint32
 * unsigned r2 = randomUint(10, 20);  // Get random in range [10, 20]
 * ```
 */
struct RandomUintGenerator {
 private:
  /// Marsaglia algorithm state
  uint32_t rngx;  ///< Marsaglia state X
  uint32_t rngy;  ///< Marsaglia state Y
  uint32_t rngz;  ///< Marsaglia state Z
  uint32_t rngc;  ///< Marsaglia carry bit

  /// Jenkins algorithm state
  uint32_t a;  ///< Jenkins state A
  uint32_t b;  ///< Jenkins state B
  uint32_t c;  ///< Jenkins state C
  uint32_t d;  ///< Jenkins state D

 public:
  /**
   * @brief Default constructor - initializes state to zero
   *
   * Note: You must call initialize() to properly seed the generator
   * before generating random numbers.
   */
  RandomUintGenerator() : rngx(0), rngy(0), rngz(0), rngc(0), a(0), b(0), c(0), d(0) {}

  /**
   * @brief Initialize and seed the random number generator
   *
   * Must be called before first use. Seeds both Marsaglia and Jenkins
   * algorithms with deterministic or non-deterministic values based on
   * configuration.
   */
  void initialize();

  /**
   * @brief Generate random unsigned 32-bit integer
   * @return Random value in range [0, RANDOM_UINT_MAX]
   */
  uint32_t operator()();

  /**
   * @brief Generate random unsigned integer in specified range
   * @param min Minimum value (inclusive)
   * @param max Maximum value (inclusive)
   * @return Random value in range [min, max]
   */
  unsigned operator()(unsigned min, unsigned max);
};

/**
 * @brief Global thread-safe random number generator
 *
 * Each thread maintains its own private instance via OpenMP threadprivate
 * pragma, eliminating lock contention in parallel regions.
 *
 * **Must call randomUint.initialize() before use!**
 */
extern RandomUintGenerator randomUint;
#pragma omp threadprivate(randomUint)

/// Maximum value returned by randomUint() (32-bit max)
constexpr uint32_t RANDOM_UINT_MAX = 0xffffffff;

}  // namespace BioSim

#endif  ///< BIOSIM4_INCLUDE_RANDOM_H_
