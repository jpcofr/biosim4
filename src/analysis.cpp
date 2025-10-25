/**
 * @file analysis.cpp
 * @brief Various reporting and analysis functions for simulation data
 *
 * Provides utilities for:
 * - Converting sensor/action enums to human-readable strings
 * - Printing genome and neural network information
 * - Calculating population statistics
 * - Generating epoch logs and reports
 */

#include "simulator.h"

#include <cassert>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <string>

namespace BioSim {

/**
 * @brief Convert sensor enum to descriptive string
 * @param sensor Sensor enum value
 * @return Human-readable sensor name (e.g., "age", "population")
 *
 * Used for detailed output and logging.
 */
std::string sensorName(Sensor sensor) {
  switch (sensor) {
    case AGE:
      return "age";
      break;
    case BOUNDARY_DIST:
      return "boundary dist";
      break;
    case BOUNDARY_DIST_X:
      return "boundary dist X";
      break;
    case BOUNDARY_DIST_Y:
      return "boundary dist Y";
      break;
    case LAST_MOVE_DIR_X:
      return "last move dir X";
      break;
    case LAST_MOVE_DIR_Y:
      return "last move dir Y";
      break;
    case LOC_X:
      return "loc X";
      break;
    case LOC_Y:
      return "loc Y";
      break;
    case LONGPROBE_POP_FWD:
      return "long probe population fwd";
      break;
    case LONGPROBE_BAR_FWD:
      return "long probe barrier fwd";
      break;
    case BARRIER_FWD:
      return "short probe barrier fwd-rev";
      break;
    case BARRIER_LR:
      return "short probe barrier left-right";
      break;
    case OSC1:
      return "osc1";
      break;
    case POPULATION:
      return "population";
      break;
    case POPULATION_FWD:
      return "population fwd";
      break;
    case POPULATION_LR:
      return "population LR";
      break;
    case RANDOM:
      return "random";
      break;
    case SIGNAL0:
      return "signal 0";
      break;
    case SIGNAL0_FWD:
      return "signal 0 fwd";
      break;
    case SIGNAL0_LR:
      return "signal 0 LR";
      break;
    case GENETIC_SIM_FWD:
      return "genetic similarity fwd";
      break;
    default:
      assert(false);
      break;
  }
}

/**
 * @brief Convert action enum to descriptive string
 * @param action Action enum value
 * @return Human-readable action name (e.g., "move east", "emit signal 0")
 *
 * Used for detailed output and logging.
 */
std::string actionName(Action action) {
  switch (action) {
    case MOVE_EAST:
      return "move east";
      break;
    case MOVE_WEST:
      return "move west";
      break;
    case MOVE_NORTH:
      return "move north";
      break;
    case MOVE_SOUTH:
      return "move south";
      break;
    case MOVE_FORWARD:
      return "move fwd";
      break;
    case MOVE_X:
      return "move X";
      break;
    case MOVE_Y:
      return "move Y";
      break;
    case SET_RESPONSIVENESS:
      return "set inv-responsiveness";
      break;
    case SET_OSCILLATOR_PERIOD:
      return "set osc1";
      break;
    case EMIT_SIGNAL0:
      return "emit signal 0";
      break;
    case KILL_FORWARD:
      return "kill fwd";
      break;
    case MOVE_REVERSE:
      return "move reverse";
      break;
    case MOVE_LEFT:
      return "move left";
      break;
    case MOVE_RIGHT:
      return "move right";
      break;
    case MOVE_RL:
      return "move R-L";
      break;
    case MOVE_RANDOM:
      return "move random";
      break;
    case SET_LONGPROBE_DIST:
      return "set longprobe dist";
      break;
    default:
      assert(false);
      break;
  }
}

/**
 * @brief Convert sensor enum to mnemonic string
 * @param sensor Sensor enum value
 * @return Short mnemonic (e.g., "Age", "Pop", "Lx")
 *
 * Useful for graph generation with tools/graph-nnet.py. Mnemonics are
 * designed to be concise for visualization while remaining recognizable.
 */
std::string sensorShortName(Sensor sensor) {
  switch (sensor) {
    case AGE:
      return "Age";
      break;
    case BOUNDARY_DIST:
      return "ED";
      break;
    case BOUNDARY_DIST_X:
      return "EDx";
      break;
    case BOUNDARY_DIST_Y:
      return "EDy";
      break;
    case LAST_MOVE_DIR_X:
      return "LMx";
      break;
    case LAST_MOVE_DIR_Y:
      return "LMy";
      break;
    case LOC_X:
      return "Lx";
      break;
    case LOC_Y:
      return "Ly";
      break;
    case LONGPROBE_POP_FWD:
      return "LPf";
      break;
    case LONGPROBE_BAR_FWD:
      return "LPb";
      break;
    case BARRIER_FWD:
      return "Bfd";
      break;
    case BARRIER_LR:
      return "Blr";
      break;
    case OSC1:
      return "Osc";
      break;
    case POPULATION:
      return "Pop";
      break;
    case POPULATION_FWD:
      return "Pfd";
      break;
    case POPULATION_LR:
      return "Plr";
      break;
    case RANDOM:
      return "Rnd";
      break;
    case SIGNAL0:
      return "Sg";
      break;
    case SIGNAL0_FWD:
      return "Sfd";
      break;
    case SIGNAL0_LR:
      return "Slr";
      break;
    case GENETIC_SIM_FWD:
      return "Gen";
      break;
    default:
      assert(false);
      break;
  }
}

/**
 * @brief Convert action enum to mnemonic string
 * @param action Action enum value
 * @return Short mnemonic (e.g., "MvE", "SG", "Mfd")
 *
 * Useful for graph generation with tools/graph-nnet.py. Mnemonics are
 * designed to be concise for visualization while remaining recognizable.
 */
std::string actionShortName(Action action) {
  switch (action) {
    case MOVE_EAST:
      return "MvE";
      break;
    case MOVE_WEST:
      return "MvW";
      break;
    case MOVE_NORTH:
      return "MvN";
      break;
    case MOVE_SOUTH:
      return "MvS";
      break;
    case MOVE_X:
      return "MvX";
      break;
    case MOVE_Y:
      return "MvY";
      break;
    case MOVE_FORWARD:
      return "Mfd";
      break;
    case SET_RESPONSIVENESS:
      return "Res";
      break;
    case SET_OSCILLATOR_PERIOD:
      return "OSC";
      break;
    case EMIT_SIGNAL0:
      return "SG";
      break;
    case KILL_FORWARD:
      return "Klf";
      break;
    case MOVE_REVERSE:
      return "Mrv";
      break;
    case MOVE_LEFT:
      return "MvL";
      break;
    case MOVE_RIGHT:
      return "MvR";
      break;
    case MOVE_RL:
      return "MRL";
      break;
    case MOVE_RANDOM:
      return "Mrn";
      break;
    case SET_LONGPROBE_DIST:
      return "LPD";
      break;
    default:
      assert(false);
      break;
  }
}

/**
 * @brief List all active sensors and actions to stdout
 *
 * Prints human-readable names of all sensors and actions that are compiled
 * into the simulator. "Active" means those appearing before NUM_SENSES and
 * NUM_ACTIONS markers in the enums. See sensors-actions.h for configuration.
 *
 * Output format:
 * @code
 * Sensors:
 *   loc X
 *   population
 *   ...
 * Actions:
 *   move X
 *   emit signal 0
 *   ...
 * @endcode
 */
void printSensorsActions() {
  unsigned i;
  std::cout << "Sensors:" << std::endl;
  for (i = 0; i < Sensor::NUM_SENSES; ++i) {
    std::cout << "  " << sensorName((Sensor)i) << std::endl;
  }
  std::cout << "Actions:" << std::endl;
  for (i = 0; i < Action::NUM_ACTIONS; ++i) {
    std::cout << "  " << actionName((Action)i) << std::endl;
  }
  std::cout << std::endl;
}

/**
 * @brief Print genome as hexadecimal gene sequences
 *
 * Outputs genome in format suitable for logging or re-parsing. Each gene
 * is printed as an 8-character hexadecimal string with 8 genes per line.
 *
 * Example output:
 * @code
 * 8a3f42d1 c0214587 5f3e9012 ... (8 genes per line)
 * a4f21038 ...
 * @endcode
 *
 * @note Each gene is exactly 4 bytes (32 bits) as verified by assert
 */
void Individual::printGenome() const {
  constexpr unsigned genesPerLine = 8;
  unsigned count = 0;
  for (Gene gene : genome) {
    if (count == genesPerLine) {
      std::cout << std::endl;
      count = 0;
    } else if (count != 0) {
      std::cout << " ";
    }

    assert(sizeof(Gene) == 4);
    uint32_t n;
    std::memcpy(&n, &gene, sizeof(n));
    std::cout << std::hex << std::setfill('0') << std::setw(8) << n;
    ++count;
  }
  std::cout << std::dec << std::endl;
}

/**
 * @brief Print neural network as edge list for graph visualization
 *
 * Outputs neural network connections in format compatible with
 * tools/graph-nnet.py for graphical visualization. Each line represents
 * one connection: source, sink, weight.
 *
 * Format:
 * @code
 * [SourceNode] [SinkNode] [Weight]
 * @endcode
 *
 * Where:
 * - Sensors: Short names (e.g., "Lx", "Pop", "Osc")
 * - Neurons: "N" + number (e.g., "N0", "N5")
 * - Actions: Short names (e.g., "MvE", "SG", "Mfd")
 *
 * Example output:
 * @code
 * Lx N0 1234
 * Pop N0 -567
 * N0 MvE 890
 * @endcode
 *
 * @see tools/graph-nnet.py for visualization script
 */
void Individual::printIGraphEdgeList() const {
  for (auto& conn : nnet.connections) {
    if (conn.sourceType == SENSOR) {
      std::cout << sensorShortName((Sensor)(conn.sourceNum));
    } else {
      std::cout << "N" << std::to_string(conn.sourceNum);
    }

    std::cout << " ";

    if (conn.sinkType == ACTION) {
      std::cout << actionShortName((Action)(conn.sinkNum));
    } else {
      std::cout << "N" << std::to_string(conn.sinkNum);
    }

    std::cout << " " << std::to_string(conn.weight) << std::endl;
  }
}

/**
 * @brief Calculate average genome length across population
 * @return Average number of genes per genome
 *
 * Samples 100 random individuals from the population and computes the mean
 * genome length. Used for tracking evolutionary trends.
 *
 * @note Samples from index range [1, population] (index 0 unused)
 */
float averageGenomeLength() {
  unsigned count = 100;
  unsigned numberSamples = 0;
  unsigned long sum = 0;

  while (count-- > 0) {
    sum += peeps[randomUint(1, parameterMngrSingleton.population)].genome.size();
    ++numberSamples;
  }
  return sum / numberSamples;
}

/**
 * @brief Append generation statistics to epoch log file
 * @param generation Generation number (0-based)
 * @param numberSurvivors Count of individuals passing survival challenge
 * @param murderCount Number of individuals killed by others (if enabled)
 *
 * Writes one line per generation to `<logDir>/epoch-log.txt` in format:
 * @code
 * generation survivors diversity avg_genome_length murders
 * @endcode
 *
 * On generation 0, creates/truncates the log file. Subsequent generations
 * append. The output format is compatible with tools/graphlog.gp for
 * plotting simulation progress.
 *
 * @note Uses hardcoded filename "epoch-log.txt" in configured log directory
 * @see geneticDiversity() for diversity calculation
 * @see averageGenomeLength() for genome length calculation
 * @todo Remove hardcoded filename
 */
void appendEpochLog(unsigned generation, unsigned numberSurvivors, unsigned murderCount) {
  std::ofstream foutput;

  if (generation == 0) {
    foutput.open(parameterMngrSingleton.logDir + "/epoch-log.txt");
    foutput.close();
  }

  foutput.open(parameterMngrSingleton.logDir + "/epoch-log.txt", std::ios::app);

  if (foutput.is_open()) {
    foutput << generation << " " << numberSurvivors << " " << geneticDiversity() << " " << averageGenomeLength() << " "
            << murderCount << std::endl;
  } else {
    assert(false);
  }
}

/**
 * @brief Print pheromone usage statistics to stdout
 *
 * Calculates and displays:
 * - Percentage of grid cells with non-zero signal
 * - Average signal magnitude across entire grid
 *
 * Only runs if at least one signal sensor (SIGNAL0, SIGNAL0_FWD, or
 * SIGNAL0_LR) is enabled. Scans all cells in pheromone layer 0.
 *
 * Output format:
 * @code
 * Signal spread X.XX%, average Y.YY
 * @endcode
 *
 * @note Only examines layer 0 of pheromone system
 */
void displaySignalUse() {
  if (Sensor::SIGNAL0 > Sensor::NUM_SENSES && Sensor::SIGNAL0_FWD > Sensor::NUM_SENSES &&
      Sensor::SIGNAL0_LR > Sensor::NUM_SENSES) {
    return;
  }

  unsigned long long sum = 0;
  unsigned count = 0;

  for (int16_t x = 0; x < parameterMngrSingleton.gridSize_X; ++x) {
    for (int16_t y = 0; y < parameterMngrSingleton.gridSize_Y; ++y) {
      unsigned magnitude = pheromones.getMagnitude(0, {x, y});
      if (magnitude != 0) {
        ++count;
        sum += magnitude;
      }
    }
  }
  std::cout << "Signal spread "
            << ((double)count / (parameterMngrSingleton.gridSize_X * parameterMngrSingleton.gridSize_Y))
            << "%, average " << ((double)sum / (parameterMngrSingleton.gridSize_X * parameterMngrSingleton.gridSize_Y))
            << std::endl;
}

/**
 * @brief Print usage statistics for sensors and actions across population
 *
 * Counts how many neural connections reference each sensor type and action
 * type across all living individuals. Helps identify which sensors/actions
 * are most useful for survival in the current challenge.
 *
 * Only scans living individuals and only counts enabled sensors/actions
 * (those before NUM_SENSES and NUM_ACTIONS markers).
 *
 * Output format:
 * @code
 * Sensors in use:
 *   count - sensor_name
 *   ...
 * Actions in use:
 *   count - action_name
 *   ...
 * @endcode
 *
 * @note Useful for understanding which inputs and outputs the population
 * has evolved to utilize
 */
void displaySensorActionReferenceCounts() {
  std::vector<unsigned> sensorCounts(Sensor::NUM_SENSES, 0);
  std::vector<unsigned> actionCounts(Action::NUM_ACTIONS, 0);

  for (unsigned index = 1; index <= parameterMngrSingleton.population; ++index) {
    if (peeps[index].alive) {
      const Individual& indiv = peeps[index];
      for (const Gene& gene : indiv.nnet.connections) {
        if (gene.sourceType == SENSOR) {
          assert(gene.sourceNum < Sensor::NUM_SENSES);
          ++sensorCounts[(Sensor)gene.sourceNum];
        }
        if (gene.sinkType == ACTION) {
          assert(gene.sinkNum < Action::NUM_ACTIONS);
          ++actionCounts[(Action)gene.sinkNum];
        }
      }
    }
  }

  std::cout << "Sensors in use:" << std::endl;
  for (unsigned i = 0; i < sensorCounts.size(); ++i) {
    if (sensorCounts[i] > 0) {
      std::cout << "  " << sensorCounts[i] << " - " << sensorName((Sensor)i) << std::endl;
    }
  }
  std::cout << "Actions in use:" << std::endl;
  for (unsigned i = 0; i < actionCounts.size(); ++i) {
    if (actionCounts[i] > 0) {
      std::cout << "  " << actionCounts[i] << " - " << actionName((Action)i) << std::endl;
    }
  }
}

/**
 * @brief Display sample genomes and neural networks from population
 * @param count Number of samples to display
 *
 * Prints detailed information for up to `count` living individuals:
 * - Individual ID
 * - Genome as hex strings (via printGenome())
 * - Neural network as edge list (via printIGraphEdgeList())
 *
 * After displaying samples, calls displaySensorActionReferenceCounts() to
 * show overall sensor/action usage statistics.
 *
 * Output format:
 * @code
 * ---------------------------
 * Individual ID 42
 * [genome hex dump]
 * [neural net edge list]
 * ---------------------------
 * [repeat for other samples]
 * [sensor/action usage stats]
 * @endcode
 *
 * @note Only displays living individuals; iterates through population
 * starting at index 1
 */
void displaySampleGenomes(unsigned count) {
  unsigned index = 1;  ///< indexes start at 1
  for (index = 1; count > 0 && index <= parameterMngrSingleton.population; ++index) {
    if (peeps[index].alive) {
      std::cout << "---------------------------\nIndividual ID " << index << std::endl;
      peeps[index].printGenome();
      std::cout << std::endl;

      /// peeps[index].printNeuralNet();
      peeps[index].printIGraphEdgeList();

      std::cout << "---------------------------" << std::endl;
      --count;
    }
  }

  displaySensorActionReferenceCounts();
}

}  // namespace BioSim
