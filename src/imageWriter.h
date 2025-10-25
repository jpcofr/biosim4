#ifndef IMAGEWRITER_H_INCLUDED
#define IMAGEWRITER_H_INCLUDED

// Creates a graphic frame for each simStep, then
// assembles them into a video at the end of a generation.

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>
#include <vector>

#include "indiv.h"
#include "params.h"
#include "peeps.h"

namespace BioSim {

// This holds all data needed to construct one image frame. The data is
// cached in this structure so that the image writer can work on it in
// a separate thread while the main thread starts a new simstep.
struct ImageFrameData {
  unsigned simStep;
  unsigned generation;
  unsigned challenge;
  unsigned barrierType;
  std::vector<Coordinate> indivLocs;
  std::vector<uint8_t> indivColors;
  std::vector<Coordinate> barrierLocs;
  typedef std::vector<std::vector<uint8_t>> SignalLayer;  // [x][y]
  std::vector<SignalLayer> signalLayers; // [layer][x][y]
};

struct ImageWriter {
  ImageWriter();
  void init(uint16_t layers, uint16_t sizeX, uint16_t sizeY);
  void startNewGeneration();
  bool saveVideoFrame(unsigned simStep, unsigned generation, unsigned challenge,
                      unsigned barrierType);
  bool saveVideoFrameSync(unsigned simStep, unsigned generation,
                          unsigned challenge, unsigned barrierType);
  void saveGenerationVideo(unsigned generation);
  void abort();
  void saveFrameThread();  // runs in a thread
  std::atomic<unsigned> droppedFrameCount;

 private:
  std::atomic<bool> busy;
  std::mutex mutex_;
  std::condition_variable condVar;
  bool dataReady;

  ImageFrameData data;
  bool abortRequested;
  unsigned skippedFrames;
};

extern ImageWriter imageWriter;

}  // namespace BioSim

#endif  // IMAGEWRITER_H_INCLUDED
