/**
 * @file genome-compare.cpp
 * @brief Genome comparison algorithms for computing similarity between genomes
 *
 * This module provides several methods to quantify the similarity between two genomes,
 * which is crucial for measuring genetic diversity in the population and tracking
 * evolutionary progress. Three comparison methods are supported:
 * - Jaro-Winkler distance (method 0): Tolerant of gaps, relocations, and length differences
 * - Hamming distance at bit level (method 1): Exact comparison requiring equal-length genomes
 * - Hamming distance at byte level (method 2): Exact comparison requiring equal-length genomes
 *
 * The comparison method is selected via the configuration parameter `genomeComparisonMethod`.
 */

#include "simulator.h"

#include <cassert>

namespace BioSim {

/**
 * @brief Check if two genes are approximately equivalent
 *
 * Determines if two genes match by comparing their source, sink, and weight properties.
 * All components must match exactly for the genes to be considered equivalent.
 *
 * @param g1 First gene to compare
 * @param g2 Second gene to compare
 * @return true if genes have matching source type, source number, sink type, sink number, and weight
 * @return false otherwise
 *
 * @note This is used as a building block for the Jaro-Winkler distance calculation
 */
bool genesMatch(const Gene& g1, const Gene& g2) {
  return g1.sinkNum == g2.sinkNum && g1.sourceNum == g2.sourceNum && g1.sinkType == g2.sinkType &&
         g1.sourceType == g2.sourceType && g1.weight == g2.weight;
}

/**
 * @brief Calculate Jaro-Winkler distance between two genomes
 *
 * Computes a similarity score between two genomes using the Jaro-Winkler distance algorithm,
 * which is tolerant of differences in genome length, gene order, and gaps. This makes it
 * ideal for comparing genomes that may have undergone mutations affecting structure.
 *
 * The algorithm works by:
 * 1. Finding matching genes within a search range
 * 2. Counting transpositions (matching genes in different positions)
 * 3. Combining these metrics into a similarity score (0.0 to 1.0)
 *
 * @param genome1 First genome to compare
 * @param genome2 Second genome to compare
 * @return float Similarity score from 0.0 (completely different) to 1.0 (identical)
 *
 * @note For performance, only the first 20 genes are compared for long genomes
 * @note Adapted from https://github.com/miguelvps/c/blob/master/jarowinkler.c (GNU GPL v3)
 *
 * @see genesMatch() for the gene equivalence criteria
 * @see genomeSimilarity() which dispatches to this function when genomeComparisonMethod == 0
 */
float jaro_winkler_distance(const Genome& genome1, const Genome& genome2) {
  float dw;
  auto max = [](int a, int b) { return a > b ? a : b; };
  auto min = [](int a, int b) { return a < b ? a : b; };

  const auto& s = genome1;
  const auto& a = genome2;

  int i, j, l;
  int m = 0, t = 0;
  int sl = s.size();  ///< strlen(s);
  int al = a.size();  ///< strlen(a);

  constexpr unsigned maxNumGenesToCompare = 20;
  sl = min(maxNumGenesToCompare,
           sl);  ///< optimization: approximate for long genomes
  al = min(maxNumGenesToCompare, al);

  std::vector<int> sflags(sl, 0);
  std::vector<int> aflags(al, 0);
  int range = max(0, max(sl, al) / 2 - 1);

  if (!sl || !al)
    return 0.0;

  /** calculate matching characters */
  for (i = 0; i < al; i++) {
    for (j = max(i - range, 0), l = min(i + range + 1, sl); j < l; j++) {
      if (genesMatch(a[i], s[j]) && !sflags[j]) {
        sflags[j] = 1;
        aflags[i] = 1;
        m++;
        break;
      }
    }
  }

  if (!m)
    return 0.0;

  /** calculate character transpositions */
  l = 0;
  for (i = 0; i < al; i++) {
    if (aflags[i] == 1) {
      for (j = l; j < sl; j++) {
        if (sflags[j] == 1) {
          l = j + 1;
          break;
        }
      }
      if (!genesMatch(a[i], s[j]))
        t++;
    }
  }
  t /= 2;

  /** Jaro distance */
  dw = (((float)m / sl) + ((float)m / al) + ((float)(m - t) / m)) / 3.0f;
  return dw;
}

/**
 * @brief Calculate bit-level Hamming distance between two genomes
 *
 * Computes similarity by comparing genomes at the bit level, counting how many bits
 * differ between the two genome representations. This provides an exact, fine-grained
 * comparison but requires genomes to be of equal length.
 *
 * The similarity score is normalized so that:
 * - Two completely random bit patterns yield ~0.5 similarity
 * - The score is scaled by 2X to map [0.0, 0.5] to [0.0, 1.0]
 * - Identical genomes yield 1.0, completely different yield 0.0
 *
 * @param genome1 First genome to compare
 * @param genome2 Second genome to compare
 * @return float Similarity score from 0.0 (all bits differ) to 1.0 (identical)
 *
 * @pre genome1.size() == genome2.size() (asserted)
 *
 * @note Uses __builtin_popcount for efficient bit counting
 * @note Result is clamped to 1.0 to handle negatively correlated patterns
 *
 * @see hammingDistanceBytes() for byte-level comparison
 * @see genomeSimilarity() which dispatches to this function when genomeComparisonMethod == 1
 */
float hammingDistanceBits(const Genome& genome1, const Genome& genome2) {
  assert(genome1.size() == genome2.size());

  const unsigned int* p1 = (const unsigned int*)genome1.data();
  const unsigned int* p2 = (const unsigned int*)genome2.data();
  const unsigned numElements = genome1.size();
  const unsigned bytesPerElement = sizeof(genome1[0]);
  const unsigned lengthBytes = numElements * bytesPerElement;
  const unsigned lengthBits = lengthBytes * 8;
  unsigned bitCount = 0;

  for (unsigned index = 0; index < genome1.size(); ++p1, ++p2, ++index) {
    bitCount += __builtin_popcount(*p1 ^ *p2);
  }

  /// For two completely random bit patterns, about half the bits will differ,
  /// resulting in c. 50% match. We will scale that by 2X to make the range
  /// from 0 to 1.0. We clip the value to 1.0 in case the two patterns are
  /// negatively correlated for some reason.
  return 1.0 - std::min(1.0, (2.0 * bitCount) / (float)lengthBits);
}

/**
 * @brief Calculate byte-level Hamming distance between two genomes
 *
 * Computes similarity by comparing genomes at the byte (Gene struct) level, counting
 * how many complete genes are identical between the two genomes. This is a coarser
 * comparison than bit-level but may be more meaningful biologically since entire
 * genes either match or don't.
 *
 * @param genome1 First genome to compare
 * @param genome2 Second genome to compare
 * @return float Similarity score from 0.0 (no matching genes) to 1.0 (all genes match)
 *
 * @pre genome1.size() == genome2.size() (asserted)
 *
 * @note This treats each Gene struct as an atomic unit (4 bytes typically)
 * @note More lenient than bit-level comparison since minor differences don't count
 *
 * @see hammingDistanceBits() for finer-grained comparison
 * @see genomeSimilarity() which dispatches to this function when genomeComparisonMethod == 2
 */
float hammingDistanceBytes(const Genome& genome1, const Genome& genome2) {
  assert(genome1.size() == genome2.size());

  const unsigned int* p1 = (const unsigned int*)genome1.data();
  const unsigned int* p2 = (const unsigned int*)genome2.data();
  const unsigned numElements = genome1.size();
  const unsigned bytesPerElement = sizeof(genome1[0]);
  const unsigned lengthBytes = numElements * bytesPerElement;
  unsigned byteCount = 0;

  for (unsigned index = 0; index < genome1.size(); ++p1, ++p2, ++index) {
    byteCount += (unsigned)(*p1 == *p2);
  }

  return byteCount / (float)lengthBytes;
}

/**
 * @brief Compute similarity between two genomes using the configured comparison method
 *
 * Dispatches to one of three comparison algorithms based on the global configuration
 * parameter `genomeComparisonMethod`. This provides a unified interface for genome
 * comparison while allowing runtime selection of the comparison strategy.
 *
 * @param g1 First genome to compare
 * @param g2 Second genome to compare
 * @return float Similarity score from 0.0 (maximally different) to 1.0 (identical)
 *
 * @note Comparison method is selected via parameterMngrSingleton.genomeComparisonMethod:
 *       - 0: Jaro-Winkler distance (handles length differences)
 *       - 1: Bit-level Hamming distance (requires equal lengths)
 *       - 2: Byte-level Hamming distance (requires equal lengths)
 *
 * @warning Methods 1 and 2 will assert-fail if genome lengths differ
 *
 * @todo Optimize by approximation for very long genomes
 *
 * @see jaro_winkler_distance()
 * @see hammingDistanceBits()
 * @see hammingDistanceBytes()
 */
float genomeSimilarity(const Genome& g1, const Genome& g2) {
  switch (parameterMngrSingleton.genomeComparisonMethod) {
    case 0:
      return jaro_winkler_distance(g1, g2);
    case 1:
      return hammingDistanceBits(g1, g2);
    case 2:
      return hammingDistanceBytes(g1, g2);
    default:
      assert(false);
  }
}

/**
 * @brief Calculate genetic diversity across the population
 *
 * Estimates population-wide genetic diversity by sampling random pairs of individuals
 * and computing the average dissimilarity of their genomes. Higher values indicate
 * more genetic variation in the population, which is generally beneficial for
 * evolutionary adaptation.
 *
 * The algorithm:
 * 1. Samples up to 1000 pairs of adjacent individuals in the population array
 * 2. Computes similarity for each pair using genomeSimilarity()
 * 3. Returns diversity as (1.0 - average_similarity)
 *
 * @return float Diversity score from 0.0 (all genomes identical) to 1.0 (maximally diverse)
 *         Returns 0.0 if population < 2
 *
 * @note Samples adjacent pairs for performance; assumes individuals are randomly distributed
 * @note Includes both living and dead individuals from the current generation
 * @note Limited to 1000 samples maximum regardless of population size
 *
 * @warning First and last elements of the peeps array are skipped to avoid boundary issues
 *
 * @see genomeSimilarity() for the underlying comparison algorithm
 */
float geneticDiversity() {
  if (parameterMngrSingleton.population < 2) {
    return 0.0;
  }

  /// count limits the number of genomes sampled for performance reasons.
  unsigned count = std::min(1000U,
                            parameterMngrSingleton.population);  ///< TODO p.analysisSampleSize;
  int numSamples = 0;
  float similaritySum = 0.0f;

  while (count > 0) {
    unsigned index0 = randomUint(1, parameterMngrSingleton.population - 1);  ///< skip first and last elements
    unsigned index1 = index0 + 1;
    similaritySum += genomeSimilarity(peeps[index0].genome, peeps[index1].genome);
    --count;
    ++numSamples;
  }
  float diversity = 1.0f - (similaritySum / numSamples);
  return diversity;
}

}  // namespace BioSim
