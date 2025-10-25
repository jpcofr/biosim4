// indiv.cpp

#include "simulator.h"

#include <cassert>
#include <iostream>

namespace BioSim {

// This is called when any individual is spawned.
// The responsiveness parameter will be initialized here to maximum value
// of 1.0, then depending on which action activation function is used,
// the default undriven value may be changed to 1.0 or action midrange.
void Individual::initialize(uint16_t index_, Coordinate loc_, Genome&& genome_) {
  index = index_;
  loc = loc_;
  // birthLoc = loc_;
  grid.set(loc_, index_);
  age = 0;
  oscPeriod = 34;  // TODO define a constant
  alive = true;
  lastMoveDir = Dir::random8();
  responsiveness = 0.5;  // range 0.0..1.0
  longProbeDist = parameterMngrSingleton.longProbeDistance;
  challengeBits = (unsigned)false;  // will be set true when some task gets accomplished
  genome = std::move(genome_);
  createWiringFromGenome();
}

}  // namespace BioSim
