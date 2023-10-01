# biosim4 - remixed (Work in Progress)

This repository is a fork of Prof. David Miller's exceptional [biosim4](https://github.com/davidrmiller/biosim4), which simulates the evolution of biological entities through natural selection. For a detailed understanding of the experiments, see the video ["I programmed some creatures. They Evolved."](https://www.youtube.com/watch?v=N3tRFayqVtk).

**Why another fork?** I want to try new approaches to structuring the code and hopefully to streamline it.

Fork's goals:
- Port the code to C++20 and prepare to use C++23 features
- Enforce code conventions
- Comment the code in detail
- Refactor the code as much as possible whenever it makes sense
- Add unit tests (with google test)
- Add benchmark tests (with google benchmnark)
- Check out for improvements in multithreading
- Find a better way to handle the config files (e.g. yaml files?)
- Find a better way to generate the output (e.g. the logic inside `imageWriter.*`)
- Enhance animation mechanisms and dependencies
- Enhance the diagrams describing the neural networks

Document Contents
-----------------

- [biosim4 - remixed (Work in Progress)](#biosim4---remixed-work-in-progress)
  - [Document Contents](#document-contents)
  - [Code walkthrough](#code-walkthrough)
    - [Main data structures](#main-data-structures)
    - [Config File](#config-file)
    - [Program Output](#program-output)
    - [Main Program Loop](#main-program-loop)
    - [Sensory Inputs and Action Outputs](#sensory-inputs-and-action-outputs)
    - [Basic Value Types](#basic-value-types)
    - [Pheromones](#pheromones)
    - [Pheromones](#pheromones-1)
    - [Useful Utility Functions](#useful-utility-functions)
  - [Building the executable](#building-the-executable)
    - [Compiling](#compiling)
      - [CMake Instructions](#cmake-instructions)
  - [Execution (UNTESTED)](#execution-untested)
  - [Tools Directory (UNTESTED)](#tools-directory-untested)


Code walkthrough<a name="CodeWalkthrough"></a>
-----------------

<a name="MainDataStructures"></a>
### Main data structures

The code in the `src` directory compiles into a single console program called `biosim4`. When invoked, it reads parameters from a default config file named `biosim4.ini`. A different config file can be specified via the command line.

The simulator initializes a 2D arena where creatures reside. The `Grid` class (see `grid.h` and `grid.cpp`) contains a 2D array of 16-bit indexes. Each non-zero index refers to a specific individual in the `Peeps` class. Zero values in `Grid` represent empty locations. `Grid` only stores indexes to indicate creature locations and has no further knowledge of the world.

The `Peeps` class (see `peeps.h` and `peeps.cpp`) stores the entire population of creatures as instances of the `Indiv` struct in a `std::vector` container. The indexes in `Grid` correspond to the positions of individuals in this vector. `Peeps` only stores the `Indiv` instances and is not aware of their internal workings.

Each creature is represented by an instance of the `Indiv` struct (see `indiv.h` and `indiv.cpp`). This struct contains the creature's genome, neural network, 2D grid location, and additional parameters such as "responsiveness" level and age. `Indiv` can convert a genome into its corresponding neural network at the beginning of the simulation. It also has output capabilities for genome and neural network data, as well as a `Indiv::getSensor()` function that computes input neurons for each simulation step.

<a name="ConfigFile"></a>
### Config File

The default config file, named `biosim4.ini`, holds all adjustable parameters for a simulation run. The `biosim4` executable reads this file at startup and monitors it for changes throughout the simulation. While not foolproof, many parameters can be adjusted during the simulation. The `ParamManager` class (see `params.h` and `params.cpp`) manages these configuration parameters and provides read-only access to them via `ParamManager::getParamRef()`.

For documentation on each parameter, refer to the provided `biosim4.ini` file. Most parameters in the config file correspond to members in the `Params` struct (see `params.h`). Additional parameters may also be stored in `Params`. Consult the documentation in `params.h` for guidance on supporting new parameters.

<a name="ProgramOutput"></a>
### Program Output

Based on the parameters set in the config file, the program can generate the following outputs:

* After each generation's completion, a line is appended to `logs/epoch.txt`. This line includes the generation number, count of survivors based on selection criteria, estimated genetic diversity, average genome length, and death count due to the "kill" gene. This file can be used with `tools/graphlog.gp` to create graphical plots.

* Sample genomes are displayed at regular intervals to `stdout`. The number and interval for these displays are specified in the config file. Genomes are shown in both hex and mnemonic formats, compatible with `tools/graph-nnet.py` for generating network diagrams.

* Movies capturing selected generations are saved in the `images/` directory. The interval for movie creation is configurable in the config file, and each movie records one generation.

* Periodic summaries are printed to `stdout`, showing the total number of neural connections in the population, categorized by each possible sensory input neuron and each possible action output neuron.

<a name="MainProgramLoop"></a>
### Main Program Loop

The simulator initiates with a call to `simulator()` in `simulator.cpp`. It starts by initializing the world and then runs three nested loops: an outer loop for each generation, a middle loop for each simulator step within that generation, and an innermost loop for each individual in the population. The innermost loop is designed to be thread-safe and can be parallelized using OpenMP.

At the end of each simulator step, the function `endOfSimStep()` is called in single-thread mode (see `endOfSimStep.cpp`). This function creates a video frame that represents the locations of all individuals at that point. The frame is then added to a stack for future movie conversion. Additional housekeeping tasks may be performed depending on the selection scenarios. For more details, refer to the comments in `endOfSimStep.cpp`.

Upon completing a generation, `endOfGeneration()` is called in single-thread mode (see `endOfGeneration.cpp`). This function converts the saved video frames into a movie. Optionally, a new graph may also be generated to display the simulation's progress. For more information, consult `endOfGeneration.cpp`.

<a name="SensoryInputsAndActionOutputs"></a>
### Sensory Inputs and Action Outputs

Refer to the [YouTube video (link above) ](https://www.youtube.com/watch?v=N3tRFayqVtk) for a description of sensory inputs and action outputs. Each sensory input and action output corresponds to a neuron in the individual's neural net brain.

The header file `sensors-actions.h` includes two enumerations: `enum Sensor`, which lists all possible sensory inputs, and `enum Action`, which lists all possible action outputs. In `enum Sensor`, any sensory inputs listed before the constant `NUM_SENSES` will be compiled into the executable. Similarly, any action outputs listed before `NUM_ACTIONS` will also be compiled. By reordering the elements within these enumerations, you can select a subset of sensory and action neurons to be included in the compiled simulator.

<a name="BasicValueTypes"></a>
### Basic Value Types

There are a few basic value types:

* `enum Compass` represents eight-way directions with enumerants N=0, NE, E, SE, S, SW, W, NW, CENTER.

* `struct Dir` serves as an abstract representation of the values in `enum Compass`.

* `struct Coord` consists of a signed 16-bit integer X,Y coordinate pair. It either represents a location in the 2D world or the difference between two locations.

* `struct Polar` contains a signed 32-bit integer magnitude and a direction of type `Dir`.

Various conversions and mathematical operations are possible between these basic types. See `unitTestBasicTypes.cpp` for examples and `basicTypes.h` for more information.

<a name="Pheromones"></a>
### Pheromones

### Pheromones

A basic system simulates pheromones emitted by individuals, referred to as "signals" in the simulator (see `signals.h` and `signals.cpp`). The `Struct Signals` contains a single layer that overlays the 2D world represented by the `Class Grid`. Each grid location can have a pheromone level, stored as an unsigned 8-bit integer. Zero indicates no pheromone, and 255 represents the maximum level. When an individual emits a pheromone, the value increases in its surrounding area up to 255. Pheromone levels decay over time unless replenished by individuals.

<a name="UsefulUtilityFunctions"></a>
### Useful Utility Functions

The function `visitNeighborhood()` in `grid.cpp` allows the execution of a user-defined lambda or function over each location in a circular neighborhood. This neighborhood is specified by a center point and a floating-point radius. The function is called once for each location, receiving a `Coord` value as an argument. Only grid-bound locations are visited, including the center. A radius of 1.0 includes the center and four immediate neighbors, while 1.5 includes eight-way neighbors. Larger radii are possible but computationally expensive.

<a name="BuildingTheExecutable"></a>
## Building the executable
-----------------
<a name="Compiling"></a>
### Compiling

#### CMake Instructions

A `CMakeLists.txt` file is provided to facilitate development, building, testing, installation, and packaging via the CMake toolchain and any IDEs that support CMake.

1. **Initial Setup and Building**: Install CMake and follow these steps to build the project.
    ```sh
    mkdir build
    cd build
    cmake ../
    cmake --build ./
    ```

2. **Test Installation and Program Execution**: To install in a test directory and run the program, execute the following commands.
    ```sh
    mkdir build
    cd build
    cmake ../
    cmake --build ./
    mkdir test_install
    cmake --install ./ --prefix ./test_install
    cd test_install
    ./bin/biosim4
    ```

3. **Creating a Release Package**: To package the release version, use the following steps.
    ```sh
    mkdir build
    cd build
    cmake ../
    cmake --build ./
    cpack ./
    ```

<a name="Execution"></a>
## Execution (UNTESTED)
------------

To verify the setup, execute either the Debug or Release version of the program from the `bin` directory, using the default configuration file, `biosim4.ini`. For instance:

```
./bin/Release/biosim4 biosim4.ini
```

The expected output should be similar to:

```
Gen 1, 2290 survivors
```

Upon successful verification, adjust the `biosim4.ini` configuration file to set your desired simulation parameters. Re-run either the Debug or Release executable. You may also specify an alternate configuration file as the first command-line argument, as shown:

```
./bin/Release/biosim4 [alternative_config.ini]
```

<a name="ToolsDirectory"></a>
## Tools Directory (UNTESTED)
-----------------

The `tools/graphlog.gp` script processes the generated log file `logs/epoch-log.txt` to produce a graphical plot of the simulation run, saved as `images/log.png`. Directory paths in `graphlog.gp` may require adjustment to fit your environment. The script can be run manually or will be invoked automatically if the "updateGraphLog" option is set to true in the simulation configuration file. See also the "updateGraphLogStride" parameter in the config file for more details.

The `tools/graph-nnet.py` script generates a neural network connection diagram using `igraph`. It takes as input a text file with the hardcoded name "net.txt," which should contain an encoded form of one genome. This format should match the one produced by `displaySampleGenomes()` in `src/analysis.cpp`, which is invoked by `simulator()` in `src/simulator.cpp`. If the "displaySampleGenomes" parameter is set to a nonzero value in the config file, a genome will be printed to stdout, and this can be copied and saved as "net.txt" to run `graph-nnet.py`.