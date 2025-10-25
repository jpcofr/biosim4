#ifndef BIOSIM4_INCLUDE_INDIV_H_
#define BIOSIM4_INCLUDE_INDIV_H_

// Indiv is the structure that represents one individual agent.

#include "basicTypes.h"
#include "genome-neurons.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace BioSim {

// Also see class Peeps.

struct Individual {
  bool alive;
  uint16_t index;  // index into peeps[] container
  Coordinate loc;  // refers to a location in grid[][]
  Coordinate birthLoc;
  /// @brief Tracks simulation steps
  unsigned age;
  Genome genome;
  NeuralNet nnet;          // derived from .genome
  float responsiveness;    // 0.0..1.0 (0 is like asleep)
  unsigned oscPeriod;      // 2..4*p.stepsPerGeneration (TBD, see executeActions())
  unsigned longProbeDist;  // distance for long forward probe for obstructions
  Dir lastMoveDir;         // direction of last movement
  unsigned challengeBits;  // modified when the indiv accomplishes some task
  std::array<float, Action::NUM_ACTIONS> feedForward(unsigned simStep);  // reads sensors, returns actions
  float getSensor(Sensor, unsigned simStep) const;
  void initialize(uint16_t index, Coordinate loc, Genome&& genome);
  void createWiringFromGenome();  // creates .nnet member from .genome member
  void printNeuralNet() const;
  void printIGraphEdgeList() const;
  void printGenome() const;
};

}  // namespace BioSim

#endif  // BIOSIM4_INCLUDE_INDIV_H_
