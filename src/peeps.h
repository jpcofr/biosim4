#ifndef PEEPS_H_INCLUDED
#define PEEPS_H_INCLUDED

#include <cstdint>
#include <vector>

#include "basicTypes.h"
#include "grid.h"
#include "indiv.h"
#include "params.h"

namespace BioSim {

struct Individual;
extern class Grid grid;

// This class keeps track of alive and dead Indiv's and where they
// are in the Grid.
// Peeps allows spawning a live Indiv at a random or specific location
// in the grid, moving Indiv's from one grid location to another, and
// killing any Indiv.
// All the Indiv instances, living and dead, are stored in the private
// .individuals member. The .cull() function will remove dead members and
// replace their slots in the .individuals container with living members
// from the end of the container for compacting the container.
// Each Indiv has an identifying index in the range 1..0xfffe that is
// stored in the Grid at the location where the Indiv resides, such that
// a Grid element value n refers to .individuals[n]. Index value 0 is
// reserved, i.e., .individuals[0] is not a valid individual.
// This class does not manage properties inside Indiv except for the
// Indiv's location in the grid and its aliveness.

/**
 * @class Peeps
 * @brief Manages alive and dead Individual instances and their positions in the
 * Grid.
 *
 * Peeps provides functionality for:
 * - Spawning a live Individual at a random or specific location in the grid.
 * - Moving Individuals from one grid location to another.
 * - Killing any Individual.
 *
 * All Individual instances, living and dead, are stored in the private
 * 'individuals' member. The 'cull()' function removes dead members and replaces
 * their slots in the 'individuals' container with living members from the end
 * of the container, compacting the container.
 *
 * Each Individual has an identifying index in the range 1..0xfffe, stored in
 * the Grid at the location where the Individual resides. Grid element value 'n'
 * refers to 'individuals[n]'. Index value 0 is reserved, i.e., 'individuals[0]'
 * is not a valid individual.
 *
 * Peeps does not manage properties inside Individual instances, except for the
 * Individual's location in the grid and its aliveness.
 */

class Peeps {
 public:
  Peeps();  // makes zero individuals
  void initialize(unsigned population);
  void queueForDeath(const Individual &);
  void drainDeathQueue();
  void queueForMove(const Individual &, Coordinate newLoc);
  void drainMoveQueue();
  unsigned deathQueueSize() const { return deathQueue.size(); }
  // getIndiv() does no error checking -- check first that loc is occupied
  Individual &getIndiv(Coordinate loc) { return individuals[grid.at(loc)]; }
  const Individual &getIndiv(Coordinate loc) const {
    return individuals[grid.at(loc)];
  }
  // Direct access:
  Individual &operator[](uint16_t index) { return individuals[index]; }
  Individual const &operator[](uint16_t index) const {
    return individuals[index];
  }

 private:
  std::vector<Individual> individuals;  // Index value 0 is reserved
  std::vector<uint16_t> deathQueue;
  std::vector<std::pair<uint16_t, Coordinate>> moveQueue;
};

}  // namespace BioSim

#endif  // PEEPS_H_INCLUDED
