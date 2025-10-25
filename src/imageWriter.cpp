/// imageWriter.cpp

#include "imageWriter.h"

#include <chrono>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>

/// Enable OpenCV support in CImg for video encoding
#define cimg_use_opencv 1
#define cimg_display 0
#include "CImg.h"
#include "simulator.h"

namespace BioSim {

cimg_library::CImgList<uint8_t> imageList;

/// Pushes a new image frame onto .imageList.
///
void saveOneFrameImmed(const ImageFrameData& data) {
  using namespace cimg_library;

  CImg<uint8_t> image(parameterMngrSingleton.gridSize_X * parameterMngrSingleton.displayScale,
                      parameterMngrSingleton.gridSize_Y * parameterMngrSingleton.displayScale,
                      1,     ///< Z depth
                      3,     ///< color channels
                      255);  ///< initial value
  uint8_t color[3];
  uint8_t temp;
  float alpha = 1.0;
  uint16_t offset;
  std::stringstream imageFilename;
  imageFilename << parameterMngrSingleton.imageDir << "/frame-" << std::setfill('0') << std::setw(6) << data.generation
                << '-' << std::setfill('0') << std::setw(6) << data.simStep << ".png";

  /// Draw save and/or unsafe area(s)
  switch (data.challenge) {
    case CHALLENGE_CENTER_WEIGHTED:
      color[0] = 0xa0;
      color[1] = 0xff;
      color[2] = 0xa0;
      image.draw_circle((parameterMngrSingleton.gridSize_X * parameterMngrSingleton.displayScale) / 2,
                        (parameterMngrSingleton.gridSize_Y * parameterMngrSingleton.displayScale) / 2,
                        (parameterMngrSingleton.gridSize_Y / 3.0 * parameterMngrSingleton.displayScale),
                        color,  ///< rgb
                        1.0);   ///< alpha

      break;
    case CHALLENGE_CENTER_UNWEIGHTED:
      color[0] = 0xa0;
      color[1] = 0xff;
      color[2] = 0xa0;
      image.draw_circle((parameterMngrSingleton.gridSize_X * parameterMngrSingleton.displayScale) / 2,
                        (parameterMngrSingleton.gridSize_Y * parameterMngrSingleton.displayScale) / 2,
                        (parameterMngrSingleton.gridSize_Y / 3.0 * parameterMngrSingleton.displayScale),
                        color,  ///< rgb
                        1.0);   ///< alpha

      break;
    case CHALLENGE_RADIOACTIVE_WALLS:
      color[0] = 0xff;
      color[1] = 0xff;
      color[2] = 0xa0;
      offset = 0;
      if (data.simStep >= parameterMngrSingleton.stepsPerGeneration / 2) {
        offset = parameterMngrSingleton.gridSize_X - 5;
      }
      image.draw_rectangle(offset * parameterMngrSingleton.displayScale, 0,
                           (offset + 5) * parameterMngrSingleton.displayScale,
                           parameterMngrSingleton.gridSize_Y * parameterMngrSingleton.displayScale,
                           color,  ///< rgb
                           1.0);   ///< alpha

      break;

    default:
      break;
  }

  /// Draw standard pheromone trails (signal layer 0)
  if (data.signalLayers.size() > 0) {
    color[0] = 0x00;
    color[1] = 0x00;
    color[2] = 0xff;
    for (int16_t x = 0; x < parameterMngrSingleton.gridSize_X; ++x) {
      for (int16_t y = 0; y < parameterMngrSingleton.gridSize_Y; ++y) {
        temp = data.signalLayers[0][x][y];
        if (temp > 0) {
          alpha = ((float)temp / 255.0) / 3.0;
          /// max alpha 0.33
          if (alpha > 0.33) {
            alpha = 0.33;
          }

          image.draw_rectangle(
              ((x - 1) * parameterMngrSingleton.displayScale) + 1,
              (((parameterMngrSingleton.gridSize_Y - y) - 2)) * parameterMngrSingleton.displayScale + 1,
              (x + 1) * parameterMngrSingleton.displayScale,
              ((parameterMngrSingleton.gridSize_Y - (y - 0))) * parameterMngrSingleton.displayScale,
              color,   ///< rgb
              alpha);  ///< alpha
        }
      }
    }
  }

  /// Draw "recent death" alarm pheromone (signal layer 1)
  /// We need to scale it up a bit, otherwise it often displays much too faint
  if (data.signalLayers.size() > 1) {
    color[0] = 0xff;
    color[1] = 0x00;
    color[2] = 0x00;
    for (int16_t x = 0; x < parameterMngrSingleton.gridSize_X; ++x) {
      for (int16_t y = 0; y < parameterMngrSingleton.gridSize_Y; ++y) {
        temp = data.signalLayers[1][x][y];
        if (temp > 0) {
          alpha = ((float)temp / 255.0) * 5;
          /// max alpha 0.5
          if (alpha > 0.5) {
            alpha = 0.5;
          }

          image.draw_rectangle(
              ((x - 1) * parameterMngrSingleton.displayScale) + 1,
              (((parameterMngrSingleton.gridSize_Y - y) - 2)) * parameterMngrSingleton.displayScale + 1,
              (x + 1) * parameterMngrSingleton.displayScale,
              ((parameterMngrSingleton.gridSize_Y - (y - 0))) * parameterMngrSingleton.displayScale,
              color,   ///< rgb
              alpha);  ///< alpha
        }
      }
    }
  }

  /// Draw barrier locations
  color[0] = color[1] = color[2] = 0x88;
  for (Coordinate loc : data.barrierLocs) {
    image.draw_rectangle(loc.x * parameterMngrSingleton.displayScale - (parameterMngrSingleton.displayScale / 2),
                         ((parameterMngrSingleton.gridSize_Y - loc.y) - 1) * parameterMngrSingleton.displayScale -
                             (parameterMngrSingleton.displayScale / 2),
                         (loc.x + 1) * parameterMngrSingleton.displayScale,
                         ((parameterMngrSingleton.gridSize_Y - (loc.y - 0))) * parameterMngrSingleton.displayScale,
                         color,  ///< rgb
                         1.0);   ///< alpha
  }

  /// Draw agents

  constexpr uint8_t maxColorVal = 0xb0;
  constexpr uint8_t maxLumaVal = 0xb0;

  auto rgbToLuma = [](uint8_t r, uint8_t g, uint8_t b) { return (r + r + r + b + g + g + g + g) / 8; };

  for (size_t i = 0; i < data.indivLocs.size(); ++i) {
    int c = data.indivColors[i];
    color[0] = (c);                ///< R: 0..255
    color[1] = ((c & 0x1f) << 3);  ///< G: 0..255
    color[2] = ((c & 7) << 5);     ///< B: 0..255

    /// Prevent color mappings to very bright colors (hard to see):
    if (rgbToLuma(color[0], color[1], color[2]) > maxLumaVal) {
      if (color[0] > maxColorVal)
        color[0] %= maxColorVal;
      if (color[1] > maxColorVal)
        color[1] %= maxColorVal;
      if (color[2] > maxColorVal)
        color[2] %= maxColorVal;
    }

    image.draw_circle(
        data.indivLocs[i].x * parameterMngrSingleton.displayScale,
        ((parameterMngrSingleton.gridSize_Y - data.indivLocs[i].y) - 1) * parameterMngrSingleton.displayScale,
        parameterMngrSingleton.agentSize,
        color,  ///< rgb
        1.0);   ///< alpha
  }

  /// Save as PNG file
  /// image.save_png(imageFilename.str().c_str(), 3);
  imageList.push_back(image);

  /// CImgDisplay local(image, "biosim3");
}

/// Starts the image writer asynchronous thread.
ImageWriter::ImageWriter() : droppedFrameCount{0}, busy{true}, dataReady{false}, abortRequested{false} {}

void ImageWriter::init(uint16_t layers, uint16_t sizeX, uint16_t sizeY) {
  /// No initialization needed for vector-based signalLayers
  startNewGeneration();
}

void ImageWriter::startNewGeneration() {
  imageList.clear();
  skippedFrames = 0;
}

uint8_t makeGeneticColor(const Genome& genome) {
  return ((genome.size() & 1) | ((genome.front().sourceType) << 1) | ((genome.back().sourceType) << 2) |
          ((genome.front().sinkType) << 3) | ((genome.back().sinkType) << 4) | ((genome.front().sourceNum & 1) << 5) |
          ((genome.front().sinkNum & 1) << 6) | ((genome.back().sourceNum & 1) << 7));
}

/// This is a synchronous gate for giving a job to saveFrameThread().
/// Called from the same thread as the main simulator loop thread during
/// single-thread mode.
/// Returns true if the image writer accepts the job; returns false
/// if the image writer is busy. Always called from a single thread
/// and communicates with a single saveFrameThread(), so no need to make
/// a critical section to safeguard the busy flag. When this function
/// sets the busy flag, the caller will immediate see it, so the caller
/// won't call again until busy is clear. When the thread clears the busy
/// flag, it doesn't matter if it's not immediately visible to this
/// function: there's no consequence other than a harmless frame-drop.
/// The condition variable allows the saveFrameThread() to wait until
/// there's a job to do.
bool ImageWriter::saveVideoFrame(unsigned simStep, unsigned generation, unsigned challenge, unsigned barrierType) {
  if (!busy) {
    busy = true;
    /// queue job for saveFrameThread()
    /// We cache a local copy of data from params, grid, and peeps because
    /// those objects will change by the main thread at the same time our
    /// saveFrameThread() is using it to output a video frame.
    data.simStep = simStep;
    data.generation = generation;
    data.challenge = challenge;
    data.barrierType = barrierType;
    data.indivLocs.clear();
    data.indivColors.clear();
    data.barrierLocs.clear();

    for (uint16_t index = 1; index <= parameterMngrSingleton.population; ++index) {
      const Individual& indiv = peeps[index];
      if (indiv.alive) {
        data.indivLocs.push_back(indiv.loc);
        data.indivColors.push_back(makeGeneticColor(indiv.genome));
      }
    }

    /// Copy signal layers
    for (unsigned layerNum = 0; layerNum < parameterMngrSingleton.signalLayers; ++layerNum) {
      for (int16_t x = 0; x < parameterMngrSingleton.gridSize_X; ++x) {
        for (int16_t y = 0; y < parameterMngrSingleton.gridSize_Y; ++y) {
          data.signalLayers[layerNum][x][y] = pheromones[layerNum][x][y];
        }
      }
    }

    auto const& barrierLocs = grid.getBarrierLocations();
    for (Coordinate loc : barrierLocs) {
      data.barrierLocs.push_back(loc);
    }

    /// tell thread there's a job to do
    {
      std::lock_guard<std::mutex> lck(mutex_);
      dataReady = true;
    }
    condVar.notify_one();
    return true;
  } else {
    /// image saver thread is busy, drop a frame
    ++droppedFrameCount;
    return false;
  }
}

/// Synchronous version, always returns true
bool ImageWriter::saveVideoFrameSync(unsigned simStep, unsigned generation, unsigned challenge, unsigned barrierType) {
  /// We cache a local copy of data from params, grid, and peeps because
  /// those objects will change by the main thread at the same time our
  /// saveFrameThread() is using it to output a video frame.
  data.simStep = simStep;
  data.generation = generation;
  data.challenge = challenge;
  data.barrierType = barrierType;
  data.indivLocs.clear();
  data.indivColors.clear();
  data.barrierLocs.clear();
  data.signalLayers.clear();

  for (uint16_t index = 1; index <= parameterMngrSingleton.population; ++index) {
    const Individual& indiv = peeps[index];
    if (indiv.alive) {
      data.indivLocs.push_back(indiv.loc);
      data.indivColors.push_back(makeGeneticColor(indiv.genome));
    }
  }

  /// Copy signal layers - note: pheromones uses Signals class [layer][x][y]
  /// but we need to copy to simple vector structure
  data.signalLayers.resize(parameterMngrSingleton.signalLayers);
  for (unsigned layerNum = 0; layerNum < parameterMngrSingleton.signalLayers; ++layerNum) {
    data.signalLayers[layerNum].resize(parameterMngrSingleton.gridSize_X);
    for (int16_t x = 0; x < parameterMngrSingleton.gridSize_X; ++x) {
      data.signalLayers[layerNum][x].resize(parameterMngrSingleton.gridSize_Y);
      for (int16_t y = 0; y < parameterMngrSingleton.gridSize_Y; ++y) {
        data.signalLayers[layerNum][x][y] = pheromones[layerNum][x][y];
      }
    }
  }

  auto const& barrierLocs = grid.getBarrierLocations();
  for (Coordinate loc : barrierLocs) {
    data.barrierLocs.push_back(loc);
  }

  saveOneFrameImmed(data);
  return true;
}

/// TODO put save_video() in its own thread
void ImageWriter::saveGenerationVideo(unsigned generation) {
  if (imageList.size() > 0) {
    std::stringstream videoFilename;
    std::string imgDir = parameterMngrSingleton.imageDir;
    /// Add trailing slash if not present
    if (!imgDir.empty() && imgDir.back() != '/') {
      imgDir += '/';
    }
    videoFilename << imgDir << "gen-" << std::setfill('0') << std::setw(6) << generation << ".avi";

    std::cout << "Saving " << imageList.size() << " frames to " << videoFilename.str() << std::endl;

    try {
      cv::setNumThreads(2);
      imageList.save_video(videoFilename.str().c_str(), 25, "H264");
      std::cout << "Video saved successfully" << std::endl;
    } catch (const std::exception& e) {
      std::cerr << "Error saving video with H264: " << e.what() << std::endl;
      std::cerr << "Trying MJPEG codec..." << std::endl;
      try {
        imageList.save_video(videoFilename.str().c_str(), 25, "MJPG");
        std::cout << "Video saved with MJPEG codec" << std::endl;
      } catch (const std::exception& e2) {
        std::cerr << "Video encoding failed: " << e2.what() << std::endl;
      }
    }

    if (skippedFrames > 0) {
      std::cout << "Video skipped " << skippedFrames << " frames" << std::endl;
    }
  }
  startNewGeneration();
}

void ImageWriter::abort() {
  busy = true;
  abortRequested = true;
  {
    std::lock_guard<std::mutex> lck(mutex_);
    dataReady = true;
  }
  condVar.notify_one();
}

/// Runs in a thread; wakes up when there's a video frame to generate.
/// When this wakes up, local copies of Params and Peeps will have been
/// cached for us to use.
void ImageWriter::saveFrameThread() {
  busy = false;  ///< we're ready for business
  std::cout << "Imagewriter thread started." << std::endl;

  while (true) {
    /// wait for job on queue
    std::unique_lock<std::mutex> lck(mutex_);
    condVar.wait(lck, [&] { return dataReady && busy; });
    /// save frame
    dataReady = false;
    busy = false;

    if (abortRequested) {
      break;
    }

    /// save image frame
    saveOneFrameImmed(imageWriter.data);

    /// std::cout << "Image writer thread waiting..." << std::endl;
    /// std::this_thread::sleep_for(std::chrono::seconds(2));
  }
  std::cout << "Image writer thread exiting." << std::endl;
}

}  // namespace BioSim
