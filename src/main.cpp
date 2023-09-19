#include <iostream>

// Basic types unit testing
#include "basicTypes.h"

// To start the simulator, call this function with argc and argv from main().
// If no command line arguments, it reads the default "biosim4.ini" config file.
// If arguments are provided, argv[1] should be the config file name.
// Additional arguments are ignored. The simulator code is in the BS namespace.
namespace BioSim {
void simulator(int argc, char **argv);
}

int main(int argc, char **argv) {
  BioSim::unitTestBasicTypes();

  // Initiate simulator with optional config (default: "biosim4.ini"). Refer to
  // simulator.cpp/h.
  BioSim::simulator(argc, argv);

  return 0;
}
