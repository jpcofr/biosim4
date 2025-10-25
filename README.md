# biosim4 - Enhanced Fork

> **Forked from [davidrmiller/biosim4](https://github.com/davidrmiller/biosim4)**
> Original author: David R. Miller | Fork enhancements: Juan Pablo Contreras Franco
> Developed with AI pair programming assistance

## About This Fork

This is an enhanced and modernized fork of the original biosim4 biological evolution simulator.
The original project simulates creatures with neural networks evolving through natural selection
in a 2D grid world.

### Original Project
- **Author**: David R. Miller
- **Repository**: https://github.com/davidrmiller/biosim4
- **Video**: ["I programmed some creatures. They evolved."](https://www.youtube.com/watch?v=N3tRFayqVtk)
- **License**: MIT License

### Fork Goals and Enhancements
- ✅ Port code to C++20/23 standards
- ✅ Enforce consistent code conventions (clang-format + pre-commit hooks)
- ✅ Comprehensive code documentation
- ✅ Refactor for improved maintainability
- ✅ Add unit tests (Google Test framework)
- 🔄 Add benchmark tests (Google Benchmark)
- 🔄 Multithreading optimizations
- ✅ Improved configuration handling
- ✅ Enhanced output generation (`imageWriter` refactoring)
- ✅ Memory leak detection and fixes
- ✅ Build system modernization (CMake + Ninja)

**Development**: This fork uses AI-assisted pair programming for code analysis, refactoring,
and implementation. All architectural decisions and code review are performed by the human developer.

---

Document Contents
-----------------

- [biosim4 - Enhanced Fork](#biosim4---enhanced-fork)
  - [About This Fork](#about-this-fork)
    - [Original Project](#original-project)
    - [Fork Goals and Enhancements](#fork-goals-and-enhancements)
  - [Document Contents](#document-contents)
  - [Code walkthrough](#code-walkthrough)
    - [Main data structures](#main-data-structures)
    - [Config file](#config-file)
    - [Program output](#program-output)
    - [Main program loop](#main-program-loop)
    - [Sensory inputs and action outputs](#sensory-inputs-and-action-outputs)
    - [Basic value types](#basic-value-types)
    - [Pheromones](#pheromones)
    - [Useful utility functions](#useful-utility-functions)
  - [Installing the code](#installing-the-code)
  - [Building the executable](#building-the-executable)
    - [System requirements](#system-requirements)
    - [Compiling](#compiling)
      - [Quick Start with Homebrew (Recommended)](#quick-start-with-homebrew-recommended)
      - [Self-Contained Build with FetchContent](#self-contained-build-with-fetchcontent)
      - [Build Options](#build-options)
      - [Memory Leak Testing](#memory-leak-testing)
      - [Troubleshooting](#troubleshooting)
      - [Generating Documentation](#generating-documentation)
  - [Execution](#execution)
    - [Running the Simulator](#running-the-simulator)
      - [With CMake Build (Recommended)](#with-cmake-build-recommended)
    - [Configuration](#configuration)
    - [Stopping the Simulator](#stopping-the-simulator)
  - [Tools directory](#tools-directory)


Code walkthrough<a name="CodeWalkthrough"></a>
--------------------

<a name="MainDataStructures"></a>
### Main data structures

The code in the src directory compiles to a single console program named biosim4. When it is
invoked, it will read parameters from a config file named config/biosim4.ini by default (or biosim4.ini in the root directory for backwards compatibility). A different
config file can be specified on the command line.

The simulator will then configure a 2D arena where the creatures live. Class Grid (see grid.h and grid.cpp)
contains a 2D array of 16-bit indexes, where each nonzero index refers to a specific individual in class Peeps (see below).
Zero values in Grid indicate empty locations. Class Grid does not know anything else about the world; it only
stores indexes to represent who lives where.

The population of creatures is stored in class Peeps (see peeps.h and peeps.cpp). Class Peeps contains
all the individuals in the simulation, stored as instances of struct Indiv in a std::vector container.
The indexes in class Grid are indexes into the vector of individuals in class Peeps. Class Peeps keeps a
container of struct Indiv, but otherwise does not know anything about the internal workings of individuals.

Each individual is represented by an instance of struct Indiv (see indiv.h and indiv.cpp). Struct Indiv
contains an individual's genome, its corresponding neural net brain, and a redundant copy of the individual's
X,Y location in the 2D grid. It also contains a few other parameters for the individual, such as its
"responsiveness" level, oscillator period, age, and other personal parameters. Struct Indiv knows how
to convert an individual's genome into its neural net brain at the beginning of the simulation.
It also knows how to print the genome and neural net brain in text format to stdout during a simulation.
It also has a function Indiv::getSensor() that is called to compute the individual's input neurons for
each simulator step.

All the simulator code lives in the BS namespace (short for "biosim".)

<a name="ConfigFile"></a>
### Config file

The config file, named config/biosim4.ini by default, contains all the tunable parameters for a
simulation run. The biosim4 executable reads the config file at startup, then monitors it for
changes during the simulation. Although it's not foolproof, many parameters can be modified during
the simulation run. Class ParamManager (see params.h and params.cpp) manages the configuration
parameters and makes them available to the simulator through a read-only pointer provided by
ParamManager::getParamRef().

See the provided config/biosim4.ini for documentation for each parameter. Most of the parameters
in the config file correspond to members in struct Params (see params.h). A few additional
parameters may be stored in struct Params. See the documentation in params.h for how to
support new parameters.


<a name="ProgramOutput"></a>
### Program output

Depending on the parameters in the config file, the following data can be produced:

* The simulator will append one line to output/logs/epoch.txt after the completion of
each generation. Each line records the generation number, number of individuals
who survived the selection criterion, an estimate of the population's genetic
diversity, average genome length, and number of deaths due to the "kill" gene.
This file can be fed to tools/graphlog.gp to produce a graphic plot.

* The simulator will display a small number of sample genomes at regular
intervals to stdout. Parameters in the config file specify the number and interval.
The genomes are displayed in hex format and also in a mnemonic format that can
be fed to tools/graph-nnet.py to produce a graphic network diagram.

* Movies of selected generations will be created in the output/images/ directory. Parameters
in the config file specify the interval at which to make movies. Each movie records
a single generation.

* At intervals, a summary is printed to stdout showing the total number of neural
connections throughout the population from each possible sensory input neuron and to each
possible action output neuron.

<a name="MainProgramLoop"></a>
### Main program loop

The simulator starts with a call to simulator() in simulator.cpp. After initializing the
world, the simulator executes three nested loops: the outer loop for each generation,
an inner loop for each simulator step within the generation, and an innermost loop for
each individual in the population. The innermost loop is thread-safe so that it can
be parallelized by OpenMP.

At the end of each simulator step, a call is made to endOfSimStep() in single-thread
mode (see endOfSimStep.cpp) to create a video frame representing the locations of all
the individuals at the end of the simulator step. The video frame is pushed on to a
stack to be converted to a movie later. Also some housekeeping may be done for certain
selection scenarios.  See the comments in endOfSimStep.cpp for more information.

At the end of each generation, a call is made to endOfGeneration() in single-thread
mode (see endOfGeneration.cpp) to create a video from the saved video frames.
Also a new graph might be generated showing the progress of the simulation. See
endOfGeneraton.cpp for more information.

<a name="SensoryInputsAndActionOutputs"></a>
### Sensory inputs and action outputs

See the YouTube video (link above) for a description of the sensory inputs and action
outputs. Each sensory input and each action output is a neuron in the individual's
neural net brain.

The header file sensors-actions.h contains enum Sensor which enumerates all the possible sensory
inputs and enum Action which enumerates all the possible action outputs.
In enum Sensor, all the sensory inputs before the enumerant NUM_SENSES will
be compiled into the executable, and all action outputs before NUM_ACTIONS
will be compiled. By rearranging the enumerants in those enums, you can select
a subset of all possible sensory and action neurons to be compiled into the
simulator.

<a name="BasicValueTypes"></a>
### Basic value types

There are a few basic value types:

* enum Compass represents eight-way directions with enumerants N=0, NE, E, SW, S, SW, W, NW, CENTER.

* struct Dir is an abstract representation of the values of enum Compass.

* struct Coord is a signed 16-bit integer X,Y coordinate pair. It is used to represent a location
in the 2D world, or can represent the difference between two locations.

* struct Polar holds a signed 32-bit integer magnitude and a direction of type Dir.

Various conversions and math are possible between these basic types. See unitTestBasicTypes.cpp
for examples. Also see basicTypes.h for more information.

<a name="Pheromones"></a>
### Pheromones

A simple system is used to simulate pheromones emitted by the individuals. Pheromones
are called "signals" in simulator-speak (see signals.h and signals.cpp). Struct Signals
holds a single layer that overlays the 2D world in class Grid. Each location can contain
a level of pheromone (there's only a single kind of pheromone supported at present). The
pheromone level at any grid location is stored as an unsigned 8-bit integer, where zero means no
pheromone, and 255 is the maximum. Each time an individual emits a pheromone, it increases
the pheromone values in a small neighborhood around the individual up to the maximum
value of 255. Pheromone levels decay over time if they are not replenished
by the individuals in the area.

<a name="UsefulUtilityFunctions"></a>
### Useful utility functions

The utility function visitNeighborhood() in grid.cpp can be used to execute a
user-defined lambda or function over each location
within a circular neighborhood defined by a center point and floating point radius. The function
calls the user-defined function once for each location, passing it a Coord value. Only locations
within the bounds of the grid are visited. The center location is included among the visited
locations. For example, a radius of 1.0 includes only the center location plus four neighboring locations.
A radius of 1.5 includes the center plus all the eight-way neighbors. The radius can be arbitrarily large
but large radii require lots of CPU cycles.



<a name="InstallingTheCode"></a>
## Installing the code
--------------------

Copy the directory structure to a location of your choice.

<a name="BuildingTheExecutable"></a>
## Building the executable
--------------------

<a name="SystemRequirements"></a>
### System requirements

- macOS (Apple Silicon or Intel)
- CMake 3.14+
- Ninja build system
- LLVM/Clang (from Homebrew)
- OpenMP support
- OpenCV 4.7+ with videoio module
- python-igraph 0.8.3 (used only by tools/graph-nnet.py)
- gnuplot 5.2.8 (used only by tools/graphlog.gp)

<a name="Compiling"></a>
### Compiling

#### Quick Start with Homebrew (Recommended)

**Fast setup: ~5 minutes**

1. Install dependencies:
```bash
brew install cmake ninja llvm libomp opencv
```

2. Set up code formatting (optional but recommended):
```bash
pip install pre-commit
pre-commit install
```

3. Build with Ninja:
```bash
rm -rf build/
mkdir build && cd build
cmake -G Ninja ..
ninja
```

4. Run:
```bash
./bin/biosim4
```

**Note on formatting**:
- `.editorconfig` provides automatic formatting hints for most editors (no plugins needed)
- Pre-commit hooks enforce C++ and Python formatting before commits (requires setup)
- See `doc/CODE_FORMATTING_MIGRATION.md` for complete details

#### Self-Contained Build with FetchContent

Use this for a fully self-contained build without Homebrew dependencies (~30-60 minutes first time):

1. Minimal dependencies:
```bash
brew install cmake ninja llvm libomp
```

2. Build with FetchContent (downloads & compiles OpenCV):
```bash
rm -rf build/
mkdir build && cd build
cmake -G Ninja -DUSE_FETCHCONTENT_OPENCV=ON ..
ninja
```

**Note:** Subsequent builds are fast - OpenCV is cached in `build/_deps/`

#### Build Options

Configure with `-D<OPTION>=<VALUE>`:

| Option                    | Default | Description                                          |
| ------------------------- | ------- | ---------------------------------------------------- |
| `USE_FETCHCONTENT_OPENCV` | `OFF`   | Build OpenCV from source                             |
| `ENABLE_VIDEO_GENERATION` | `ON`    | Enable .avi video file generation                    |
| `ENABLE_SANITIZERS`       | `OFF`   | Enable AddressSanitizer & UndefinedBehaviorSanitizer |
| `ENABLE_THREAD_SANITIZER` | `OFF`   | Enable ThreadSanitizer                               |
| `BUILD_DOCUMENTATION`     | `OFF`   | Build Doxygen documentation (requires doxygen)       |

Examples:
```bash
# Default: use Homebrew OpenCV with videos enabled
cmake -G Ninja ..

# Build OpenCV from source
cmake -G Ninja -DUSE_FETCHCONTENT_OPENCV=ON ..

# Build with memory leak detection
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..

# Build with race condition detection
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_THREAD_SANITIZER=ON ..

# Disable video generation
cmake -G Ninja -DENABLE_VIDEO_GENERATION=OFF ..
```

#### Memory Leak Testing

Build with AddressSanitizer:
```bash
rm -rf build/
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON ..
ninja
cd ..
./build/bin/biosim4 tests/configs/leak-test.ini
```

Use the automated testing script:
```bash
./scripts/test-leaks.sh
```

#### Troubleshooting

**"OpenCV not found"**
```bash
# Check if OpenCV is installed
brew list opencv

# If not installed
brew install opencv
```

**Clean Rebuild**
```bash
rm -rf build/
mkdir build && cd build
cmake -G Ninja ..
ninja
```

**Verify Video Support**
```bash
pkg-config --modversion opencv4
./src/biosim4  # Check for .avi files in output/images/
```

#### Generating Documentation

The project uses Doxygen to generate comprehensive API documentation from source code comments.

**Prerequisites:**
```bash
# Install Doxygen and Graphviz (for dependency graphs)
brew install doxygen graphviz
```

**Build Documentation:**
```bash
cd build
cmake -G Ninja -DBUILD_DOCUMENTATION=ON ..
ninja docs
```

**View Documentation:**
```bash
# Open in default browser
open docs/html/index.html
```

The documentation includes:
- Class and struct hierarchies with inheritance diagrams
- Function and method documentation
- File organization and dependencies
- Source code browser with cross-references
- Call graphs and collaboration diagrams
- Search functionality

**Configuration:** The `Doxyfile` in the project root controls documentation generation settings. Key features enabled:
- Extracts documentation from all source files (even undocumented members)
- Includes `src/` and `include/` directories
- Excludes third-party code (`CImg.h`), build artifacts, and test files
- Generates interactive HTML with tree view navigation
- Creates class diagrams and include graphs (requires Graphviz)
- **Output location**: `build/docs/` (automatically excluded from searches and git)

**Note:** Generated documentation is in `build/docs/html/` and automatically excluded from version control and workspace searches.

<a name="Execution"></a>
## Execution
--------------------

### Running the Simulator

#### With CMake Build (Recommended)

**Important**: Always run the simulator from the project root directory:

```sh
cd /path/to/biosim4    # Navigate to project root if not already there
./build/bin/biosim4
```

The simulator will automatically look for the configuration file in `config/biosim4.ini` (or `biosim4.ini` in the root directory for backwards compatibility), and will write output files to `output/images/` and `output/logs/` directories relative to the current directory.

Optionally, specify a different config file:

```sh
./build/bin/biosim4 path/to/config.ini
```

**Common mistake**: Don't run from inside the `build/` directory:
```sh
# ❌ WRONG - will fail to find config file
cd build/bin && ./biosim4

# ✅ CORRECT - run from project root
./build/bin/biosim4
```

### Configuration

The simulator reads parameters from `config/biosim4.ini` by default (or `biosim4.ini` in the root directory for backwards compatibility). You can also specify a different config file: Key parameters include:

- **maxGenerations**: Number of generations to simulate (default: 200000)
  - Set to a lower value (e.g., 100-500) for quick tests
- **saveVideo**: Enable/disable video generation (default: true)
- **videoStride**: Generate a video every N generations (default: 25)
- **videoSaveFirstFrames**: Always save videos for first N generations (default: 2)
- **imageDir**: Directory where videos are saved (default: output/images)
- **population**: Number of individuals per generation (default: 3000)
- **stepsPerGeneration**: Simulation steps per generation (default: 300)

Generated videos will be saved as `gen-NNNNNN.avi` in the `output/images/` directory.

### Stopping the Simulator

The simulator runs until it reaches `maxGenerations` or you manually stop it:

- Press `Ctrl+C` in the terminal to stop the simulation
- Or use `kill <pid>` if running in background


<a name="ToolsDirectory"></a>
## Tools directory
--------------------

tools/graphlog.gp takes the generated log file output/logs/epoch-log.txt
and generates a graphic plot of the simulation run in output/images/log.png. You may need to adjust
the directory paths in graphlog.gp for your environment. graphlog.gp can be invoked manually,
or if the option "updateGraphLog" is set to true
in the simulation config file, the simulator will try to invoke tools/graphlog.gp automatically
during the simulation run. Also see the parameter named updateGraphLogStride in the config file.

tools/graph-nnet.py takes a text file (hardcoded name "net.txt") and generates a neural net
connection diagram using igraph. The file net.txt contains an encoded form of one genome, and
must be the same format as the files
generated by displaySampleGenomes() in src/analysis.cpp which is called by simulator() in
src/simulator.cpp. The genome output is printed to stdout automatically
if the parameter named "displaySampleGenomes" is set to nonzero in the config file.
An individual genome can be copied from that output stream and renamed "net.txt" in order to run
graph-nnet.py.
