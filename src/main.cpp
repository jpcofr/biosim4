#include <iostream>

/**
 * @brief Main simulator entry point.
 *
 * Starts the simulator with command line arguments.
 * - If no command line arguments are provided, reads the default "config/biosim4.ini" config file.
 * - If arguments are provided, argv[1] should be the config file name.
 * - Additional arguments are ignored.
 *
 * @param argc Argument count
 * @param argv Argument vector
 */
namespace BioSim {
void simulator(int argc, char** argv);
}

/**
 * @brief Program entry point.
 *
 * Initiates the simulator with optional config file parameter.
 * Default configuration file: "config/biosim4.ini"
 *
 * @param argc Argument count
 * @param argv Argument vector (argv[1] = config file path if provided)
 * @return Exit code (0 for success)
 *
 * @see BioSim::simulator()
 */
int main(int argc, char** argv) {
  BioSim::simulator(argc, argv);

  return 0;
}
