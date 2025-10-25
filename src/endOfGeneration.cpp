// endOfGeneration.cpp

#include "imageWriter.h"
#include "simulator.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <iostream>
#include <utility>

namespace BioSim {

// At the end of each generation, we save a video file (if p.saveVideo is true)
// and print some genomic statistics to stdout (if p.updateGraphLog is true).

void endOfGeneration(unsigned generation) {
  {
    if (parameterMngrSingleton.saveVideo && ((generation % parameterMngrSingleton.videoStride) == 0 ||
                                             generation <= parameterMngrSingleton.videoSaveFirstFrames ||
                                             (generation >= parameterMngrSingleton.parameterChangeGenerationNumber &&
                                              generation <= parameterMngrSingleton.parameterChangeGenerationNumber +
                                                                parameterMngrSingleton.videoSaveFirstFrames))) {
      imageWriter.saveGenerationVideo(generation);
    }
  }

  {
    if (parameterMngrSingleton.updateGraphLog &&
        (generation == 1 || ((generation % parameterMngrSingleton.updateGraphLogStride) == 0))) {
#pragma GCC diagnostic ignored "-Wunused-result"
      std::system(parameterMngrSingleton.graphLogUpdateCommand.c_str());
    }
  }
}

}  // namespace BioSim
