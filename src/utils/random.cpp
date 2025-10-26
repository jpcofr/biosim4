/**
 * @file random.cpp
 * @brief Implementation of fast pseudo-random number generation for simulation
 *
 * This file implements thread-safe random number generation using two algorithms:
 * - **Marsaglia KISS** (Keep It Simple Stupid) algorithm
 * - **Jenkins** small fast algorithm
 *
 * ## Thread Safety
 * The global `randomUint` instance is declared `threadprivate` for OpenMP,
 * meaning each thread instantiates its own private copy. This eliminates
 * lock contention but prevents trivial constructors.
 *
 * ## Initialization Requirements
 * Because of the threadprivate constraint, the RNG uses an explicit
 * `initialize()` method rather than a constructor. This must be called:
 * - After reading config parameters (in `simulator()`)
 * - Before any random number generation
 * - Once per thread
 *
 * ## Configuration
 * Controlled by `config/biosim4.toml` parameters:
 * - `deterministic`: If true, use `RNGSeed` for reproducible sequences
 * - `RNGSeed`: Seed value when deterministic mode is enabled
 *
 * @see RandomUintGenerator
 * @see simulator.cpp
 */

#include "random.h"

#include "../core/simulation/simulator.h"  // For parameterMngrSingleton
#include "omp.h"

#include <cassert>
#include <chrono>
#include <climits>
#include <cmath>
#include <random>

namespace BioSim {
inline namespace v1 {
namespace Utils {

/**
 * @brief Initialize the random number generator with appropriate seeds
 *
 * Seeds both the Marsaglia and Jenkins algorithm state variables. The seeding
 * strategy depends on the `deterministic` configuration parameter:
 *
 * ## Deterministic Mode (deterministic=true)
 * - Uses `RNGSeed` parameter as base seed
 * - Each thread gets a unique but reproducible seed: `RNGSeed + thread_num`
 * - Guarantees identical sequences across runs with same seed
 * - All state variables forced to non-zero (required by algorithms)
 *
 * ## Non-Deterministic Mode (deterministic=false)
 * - Uses `std::mt19937` seeded with `time(0) + thread_num`
 * - Each thread gets unique, non-reproducible sequences
 * - Ensures all state variables are non-zero via rejection sampling
 *
 * @note Must be called after config parameters are loaded and before any
 *       calls to operator(). Typically invoked in `simulator()`.
 * @note Thread-safe: each thread initializes its own threadprivate instance
 *
 * @warning Marsaglia and Jenkins algorithms fail if any state is zero,
 *          so zero values are explicitly avoided during initialization
 *
 * @see parameterMngrSingleton
 */
void RandomUintGenerator::initialize() {
  if (parameterMngrSingleton.deterministic) {
    // Initialize Marsaglia KISS algorithm state
    // Overflow wrap-around is acceptable - we just need unrelated values
    // Each thread uses a different but deterministic seed
    // Zero values are forced to arbitrary non-zero (required by algorithm)
    rngx = parameterMngrSingleton.RNGSeed + 123456789 + omp_get_thread_num();
    rngy = parameterMngrSingleton.RNGSeed + 362436000 + omp_get_thread_num();
    rngz = parameterMngrSingleton.RNGSeed + 521288629 + omp_get_thread_num();
    rngc = parameterMngrSingleton.RNGSeed + 7654321 + omp_get_thread_num();
    rngx = rngx != 0 ? rngx : 123456789;
    rngy = rngy != 0 ? rngy : 123456789;
    rngz = rngz != 0 ? rngz : 123456789;
    rngc = rngc != 0 ? rngc : 123456789;

    // Initialize Jenkins algorithm state deterministically per-thread
    a = 0xf1ea5eed;
    b = c = d = parameterMngrSingleton.RNGSeed + omp_get_thread_num();
    if (b == 0) {
      b = c = d + 123456789;
    }
  } else {
    // Non-deterministic initialization using mt19937 (Mersenne Twister)
    // Strategy:
    // 1. Seed mt19937 with time(0) + thread_num for per-thread uniqueness
    //    (time() alone has coarse resolution - threads might init simultaneously)
    // 2. Use mt19937 to generate starting coefficients for Marsaglia and Jenkins
    // 3. Reject any zero values via do-while loops
    std::mt19937 generator(time(0) + omp_get_thread_num());

    // Initialize Marsaglia state, rejecting zero values
    do {
      rngx = generator();
    } while (rngx == 0);
    do {
      rngy = generator();
    } while (rngy == 0);
    do {
      rngz = generator();
    } while (rngz == 0);
    do {
      rngc = generator();
    } while (rngc == 0);

    // Initialize Jenkins state, rejecting zero values
    a = 0xf1ea5eed;
    do {
      b = c = d = generator();
    } while (b == 0);
  }
}

/**
 * @brief Generate a random 32-bit unsigned integer
 *
 * Returns uniformly distributed random values across the full uint32 range.
 * Currently uses the Jenkins algorithm (controlled by `if (false)` branch).
 *
 * ## Algorithm Selection
 * - **Jenkins**: Fast, good statistical properties (currently active)
 * - **Marsaglia KISS**: Alternative with similar quality (disabled)
 *
 * ## Performance vs Quality Trade-off
 * Neither algorithm is cryptographically secure, but that's intentional:
 * - Speed is critical (called in deeply nested loops)
 * - "Shotgun quality" randomness is sufficient for simulation
 * - Jenkins is empirically the fastest
 *
 * @return Random 32-bit unsigned integer in range [0, UINT32_MAX]
 *
 * @note The Marsaglia implementation is from:
 *       http://www0.cs.ucl.ac.uk/staff/d.jones/GoodPracticeRNG.pdf
 *       (attributed to G. Marsaglia)
 *
 * @see operator()(unsigned, unsigned) for bounded random values
 */
uint32_t RandomUintGenerator::operator()() {
  if (false) {
    // Marsaglia KISS algorithm (currently disabled in favor of Jenkins)
    uint64_t t, a = 698769069ULL;
    rngx = 69069 * rngx + 12345;
    rngy ^= (rngy << 13);
    rngy ^= (rngy >> 17);
    rngy ^= (rngy << 5);  // CRITICAL: y must never be set to zero!
    t = a * rngz + rngc;
    rngc = (t >> 32);  // CRITICAL: Also avoid setting z=c=0!
    return rngx + rngy + (rngz = t);
  } else {
    // Jenkins small fast algorithm (currently active)
#define rot32(x, k) (((x) << (k)) | ((x) >> (32 - (k))))
    uint32_t e = a - rot32(b, 27);
    a = b ^ rot32(c, 17);
    b = c + d;
    c = d + e;
    d = e + a;
    return d;
  }
}

/**
 * @brief Generate a random unsigned integer within a specified range
 *
 * Returns uniformly distributed random values in the closed interval [min, max].
 *
 * ## Implementation Note: Modulo Bias
 * Uses the modulus operator for range mapping, which introduces slight bias
 * when `(max - min + 1)` is not a power of two. This is **intentionally accepted**:
 * - The bias is negligible for simulation purposes
 * - Speed is critical (called in deeply nested loops)
 * - Alternative approaches (std::uniform_int_distribution) have overhead
 *
 * ## Performance Priority
 * This function prioritizes execution speed over perfect statistical uniformity.
 * For simulation randomness, "good enough" is genuinely good enough.
 *
 * @param min Minimum value (inclusive)
 * @param max Maximum value (inclusive)
 * @return Random unsigned integer in range [min, max]
 *
 * @pre max >= min (enforced by assertion)
 * @note Typical bias magnitude: ~(UINT32_MAX % range) / UINT32_MAX
 *
 * @see operator()() for unbounded random values
 */
unsigned RandomUintGenerator::operator()(unsigned min, unsigned max) {
  assert(max >= min);
  return ((*this)() % (max - min + 1)) + min;
}

/**
 * @var randomUint
 * @brief Global thread-safe random number generator instance
 *
 * This is the primary RNG used throughout the simulation. The `threadprivate`
 * pragma ensures each OpenMP thread maintains its own independent instance,
 * eliminating lock contention while preserving deterministic behavior per thread.
 *
 * ## Thread Safety Model
 * - Each thread has a private copy (no shared state)
 * - No locks or atomic operations needed
 * - Each thread's sequence is independent and reproducible (when deterministic)
 *
 * ## Usage Pattern
 * ```cpp
 * // In simulator.cpp after config load:
 * randomUint.initialize();
 *
 * // Anywhere in the simulation:
 * uint32_t val = randomUint();          // Full range
 * unsigned bounded = randomUint(0, 99); // Range [0, 99]
 * ```
 *
 * @note Must call `initialize()` before first use
 * @see RandomUintGenerator::initialize()
 */
RandomUintGenerator randomUint;
#pragma omp threadprivate(randomUint)

}  // namespace Utils
}  // namespace v1
}  // namespace BioSim
