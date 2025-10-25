/**
 * @file raylibRenderBackend.cpp
 * @brief Raylib-based implementation of the rendering backend interface.
 *
 * This file provides a concrete implementation of IRenderBackend using the raylib
 * library for image manipulation and frame buffer management, with FFmpeg library
 * integration for professional-quality video encoding.
 *
 * Video Encoding Strategy:
 * - raylib provides Image manipulation for frame rendering
 * - FFmpeg libavcodec/libavformat libraries handle video encoding
 * - Direct API usage (no external process calls)
 * - H.264 codec with MP4/AVI container support
 */

#include "params.h"
#include "renderBackend.h"

#include <cmath>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

// Raylib includes
#include "raylib.h"

// FFmpeg includes (C API, use extern "C")
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

namespace BioSim {

/**
 * @class RaylibRenderBackend
 * @brief Concrete rendering backend using raylib library.
 *
 * This class wraps raylib image operations in the IRenderBackend interface.
 * It maintains a vector of raylib Images to buffer frames and provides drawing
 * primitives that match the IRenderBackend contract.
 *
 * Implementation Details:
 * - Uses raylib Image (PIXELFORMAT_UNCOMPRESSED_R8G8B8A8)
 * - Y-axis uses simulation convention (0=bottom, max=top)
 * - Alpha blending done via manual pixel operations for transparency
 * - Video encoding via FFmpeg libavcodec/libavformat APIs (no external processes)
 * - H.264 codec with AVI container (MPEG-4 fallback if H.264 unavailable)
 *
 * Thread Safety:
 * - NOT thread-safe - caller must synchronize access
 * - Single-threaded use intended (sync mode in ImageWriter)
 */
class RaylibRenderBackend : public IRenderBackend {
 public:
  RaylibRenderBackend();
  ~RaylibRenderBackend() override;

  void init(uint16_t gridWidth, uint16_t gridHeight, uint16_t displayScale, uint16_t agentSize) override;
  void startNewGeneration() override;
  void beginFrame(unsigned simStep, unsigned generation) override;
  void drawChallengeZone(ChallengeZoneType zoneType, unsigned simStep, unsigned stepsPerGeneration) override;
  void drawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const Color& color) override;
  void drawCircle(int16_t centerX, int16_t centerY, uint16_t radius, const Color& color) override;
  void endFrame() override;
  bool saveVideo(unsigned generation, const std::string& outputPath) override;
  size_t getFrameCount() const override;

 private:
  /**
   * @brief Convert simulation Y coordinate to screen Y coordinate.
   *
   * Raylib uses top-left origin (Y increases downward) by default.
   * Simulation uses bottom-left origin (Y increases upward).
   * This helper inverts the Y axis.
   *
   * @param simY Simulation Y coordinate (0 = bottom of grid)
   * @return Screen Y coordinate (0 = top of image)
   */
  int toScreenY(int16_t simY) const;

  /**
   * @brief Convert BioSim::Color to raylib Color.
   */
  ::Color toRaylibColor(const Color& color) const;

  /**
   * @brief Free all buffered frames.
   */
  void clearFrameBuffer();

  /**
   * @brief Encode frames to video using FFmpeg API (no external process calls).
   *
   * @param generation Generation number for filename
   * @param outputPath Base directory for output
   * @return true if successful, false otherwise
   */
  bool exportAndEncodeVideo(unsigned generation, const std::string& outputPath);

  uint16_t gridWidth_;     ///< Simulation grid width in cells
  uint16_t gridHeight_;    ///< Simulation grid height in cells
  uint16_t displayScale_;  ///< Pixel scaling factor (pixels per grid cell)
  uint16_t agentSize_;     ///< Agent circle radius in pixels
  int imageWidth_;         ///< Image width in pixels
  int imageHeight_;        ///< Image height in pixels

  Image currentFrame_;              ///< Frame currently being drawn
  std::vector<Image> frameBuffer_;  ///< Accumulated frames for video
  bool frameInProgress_;            ///< True if beginFrame() called without endFrame()
};

RaylibRenderBackend::RaylibRenderBackend()
    : gridWidth_(0),
      gridHeight_(0),
      displayScale_(1),
      agentSize_(1),
      imageWidth_(0),
      imageHeight_(0),
      frameInProgress_(false) {
  // Note: We don't call InitWindow() since we're running headless
  // raylib image functions work without window initialization
}

RaylibRenderBackend::~RaylibRenderBackend() {
  clearFrameBuffer();
  if (frameInProgress_) {
    UnloadImage(currentFrame_);
  }
}

void RaylibRenderBackend::init(uint16_t gridWidth, uint16_t gridHeight, uint16_t displayScale, uint16_t agentSize) {
  gridWidth_ = gridWidth;
  gridHeight_ = gridHeight;
  displayScale_ = displayScale;
  agentSize_ = agentSize;
  imageWidth_ = gridWidth * displayScale;
  imageHeight_ = gridHeight * displayScale;
  startNewGeneration();
}

void RaylibRenderBackend::startNewGeneration() {
  clearFrameBuffer();
  frameInProgress_ = false;
}

void RaylibRenderBackend::beginFrame(unsigned simStep, unsigned generation) {
  if (frameInProgress_) {
    std::cerr << "Warning: beginFrame() called without endFrame(). Discarding previous frame." << std::endl;
    UnloadImage(currentFrame_);
  }

  // Create blank white canvas (use raylib's Color type explicitly)
  currentFrame_ = GenImageColor(imageWidth_, imageHeight_, ::Color{255, 255, 255, 255});
  frameInProgress_ = true;

  // Metadata is stored but not drawn (used for debugging/logging)
  (void)simStep;     // unused for now
  (void)generation;  // unused for now
}

void RaylibRenderBackend::drawChallengeZone(ChallengeZoneType zoneType, unsigned simStep, unsigned stepsPerGeneration) {
  if (!frameInProgress_) {
    std::cerr << "Error: drawChallengeZone() called before beginFrame()" << std::endl;
    return;
  }

  switch (zoneType) {
    case ChallengeZoneType::CENTER_WEIGHTED:
    case ChallengeZoneType::CENTER_UNWEIGHTED: {
      // Green circle in center (safe zone)
      int centerX = imageWidth_ / 2;
      int centerY = imageHeight_ / 2;
      int radius = static_cast<int>(gridHeight_ * displayScale_ / 3.0f);
      ::Color green = {0xa0, 0xff, 0xa0, 0xff};
      ImageDrawCircle(&currentFrame_, centerX, centerY, radius, green);
      break;
    }

    case ChallengeZoneType::RADIOACTIVE_WALLS: {
      // Yellow wall on left half, then right half of generation
      ::Color yellow = {0xff, 0xff, 0xa0, 0xff};
      int wallWidth = 5 * displayScale_;
      int xOffset = 0;

      if (simStep >= stepsPerGeneration / 2) {
        xOffset = imageWidth_ - wallWidth;
      }

      ImageDrawRectangle(&currentFrame_, xOffset, 0, wallWidth, imageHeight_, yellow);
      break;
    }

    case ChallengeZoneType::NONE:
    default:
      // No challenge zone to draw
      break;
  }
}

void RaylibRenderBackend::drawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const Color& color) {
  if (!frameInProgress_) {
    std::cerr << "Error: drawRectangle() called before beginFrame()" << std::endl;
    return;
  }

  // Convert grid coordinates to pixel coordinates
  int px1 = x1 * displayScale_;
  int py1 = toScreenY(y1);
  int px2 = x2 * displayScale_;
  int py2 = toScreenY(y2);

  // Ensure px1 < px2 and py1 < py2
  if (px1 > px2)
    std::swap(px1, px2);
  if (py1 > py2)
    std::swap(py1, py2);

  int width = px2 - px1;
  int height = py2 - py1;

  ::Color rColor = toRaylibColor(color);

  // Handle alpha blending manually if alpha < 255
  if (color.a < 255) {
    // For semi-transparent drawing, we need to blend manually
    // raylib's ImageDrawRectangle doesn't handle alpha well, so we draw pixel by pixel
    // This is slower but necessary for pheromone trails
    for (int y = py1; y < py1 + height; ++y) {
      for (int x = px1; x < px1 + width; ++x) {
        if (x >= 0 && x < imageWidth_ && y >= 0 && y < imageHeight_) {
          ::Color existing = GetImageColor(currentFrame_, x, y);
          float alpha = color.a / 255.0f;
          ::Color blended = {static_cast<unsigned char>(color.r * alpha + existing.r * (1.0f - alpha)),
                             static_cast<unsigned char>(color.g * alpha + existing.g * (1.0f - alpha)),
                             static_cast<unsigned char>(color.b * alpha + existing.b * (1.0f - alpha)), 255};
          ImageDrawPixel(&currentFrame_, x, y, blended);
        }
      }
    }
  } else {
    // Fully opaque, use fast rectangle draw
    ImageDrawRectangle(&currentFrame_, px1, py1, width, height, rColor);
  }
}

void RaylibRenderBackend::drawCircle(int16_t centerX, int16_t centerY, uint16_t radius, const Color& color) {
  if (!frameInProgress_) {
    std::cerr << "Error: drawCircle() called before beginFrame()" << std::endl;
    return;
  }

  // Convert grid coordinates to pixel coordinates
  int px = centerX * displayScale_;
  int py = toScreenY(centerY);

  ::Color rColor = toRaylibColor(color);

  // Handle alpha blending for semi-transparent circles
  if (color.a < 255) {
    // Manual alpha blending for circles
    // Draw circle pixel by pixel with alpha blending
    float alpha = color.a / 255.0f;
    for (int y = -radius; y <= radius; ++y) {
      for (int x = -radius; x <= radius; ++x) {
        if (x * x + y * y <= radius * radius) {
          int pixelX = px + x;
          int pixelY = py + y;
          if (pixelX >= 0 && pixelX < imageWidth_ && pixelY >= 0 && pixelY < imageHeight_) {
            ::Color existing = GetImageColor(currentFrame_, pixelX, pixelY);
            ::Color blended = {static_cast<unsigned char>(color.r * alpha + existing.r * (1.0f - alpha)),
                               static_cast<unsigned char>(color.g * alpha + existing.g * (1.0f - alpha)),
                               static_cast<unsigned char>(color.b * alpha + existing.b * (1.0f - alpha)), 255};
            ImageDrawPixel(&currentFrame_, pixelX, pixelY, blended);
          }
        }
      }
    }
  } else {
    // Fully opaque, use fast circle draw
    ImageDrawCircle(&currentFrame_, px, py, radius, rColor);
  }
}

void RaylibRenderBackend::endFrame() {
  if (!frameInProgress_) {
    std::cerr << "Warning: endFrame() called without beginFrame()" << std::endl;
    return;
  }

  // Add completed frame to buffer
  // Note: We need to make a copy since Image is a struct with pointers
  Image frameCopy = ImageCopy(currentFrame_);
  frameBuffer_.push_back(frameCopy);

  // Clean up current frame
  UnloadImage(currentFrame_);
  frameInProgress_ = false;
}

bool RaylibRenderBackend::saveVideo(unsigned generation, const std::string& outputPath) {
  if (frameBuffer_.empty()) {
    std::cerr << "Warning: No frames to save for generation " << generation << std::endl;
    return false;
  }

  std::cout << "Saving " << frameBuffer_.size() << " frames for generation " << generation << std::endl;

  return exportAndEncodeVideo(generation, outputPath);
}

size_t RaylibRenderBackend::getFrameCount() const {
  return frameBuffer_.size();
}

int RaylibRenderBackend::toScreenY(int16_t simY) const {
  // Invert Y axis: sim (0=bottom) → screen (0=top)
  return imageHeight_ - ((simY + 1) * displayScale_);
}

::Color RaylibRenderBackend::toRaylibColor(const Color& color) const {
  return ::Color{color.r, color.g, color.b, color.a};
}

void RaylibRenderBackend::clearFrameBuffer() {
  for (auto& frame : frameBuffer_) {
    UnloadImage(frame);
  }
  frameBuffer_.clear();
}

bool RaylibRenderBackend::exportAndEncodeVideo(unsigned generation, const std::string& outputPath) {
  // Build output video path
  std::stringstream videoPath;
  videoPath << outputPath << "/gen-" << std::setfill('0') << std::setw(6) << generation << ".avi";
  std::string outputFile = videoPath.str();

  std::cout << "Encoding " << frameBuffer_.size() << " frames using FFmpeg API..." << std::endl;

  // Initialize FFmpeg codec
  const AVCodec* codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec) {
    // Fallback to MPEG4 if H264 not available
    codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    if (!codec) {
      std::cerr << "Error: No suitable video codec found (H264/MPEG4)" << std::endl;
      return false;
    }
    std::cout << "Using MPEG-4 codec (H.264 not available)" << std::endl;
  }

  AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
  if (!codecCtx) {
    std::cerr << "Error: Could not allocate codec context" << std::endl;
    return false;
  }

  // Configure codec parameters
  codecCtx->bit_rate = 400000;
  codecCtx->width = imageWidth_;
  codecCtx->height = imageHeight_;
  codecCtx->time_base = {1, 25};  // 25 FPS
  codecCtx->framerate = {25, 1};
  codecCtx->gop_size = 10;
  codecCtx->max_b_frames = 1;
  codecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

  // Set codec-specific options
  if (codec->id == AV_CODEC_ID_H264) {
    av_opt_set(codecCtx->priv_data, "preset", "medium", 0);
    av_opt_set(codecCtx->priv_data, "crf", "23", 0);
  }

  // Open codec
  if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
    std::cerr << "Error: Could not open codec" << std::endl;
    avcodec_free_context(&codecCtx);
    return false;
  }

  // Create output format context
  AVFormatContext* formatCtx = nullptr;
  avformat_alloc_output_context2(&formatCtx, nullptr, "avi", outputFile.c_str());
  if (!formatCtx) {
    std::cerr << "Error: Could not create format context" << std::endl;
    avcodec_free_context(&codecCtx);
    return false;
  }

  // Add video stream
  AVStream* stream = avformat_new_stream(formatCtx, nullptr);
  if (!stream) {
    std::cerr << "Error: Could not create stream" << std::endl;
    avformat_free_context(formatCtx);
    avcodec_free_context(&codecCtx);
    return false;
  }

  stream->time_base = codecCtx->time_base;
  avcodec_parameters_from_context(stream->codecpar, codecCtx);

  // Open output file
  if (!(formatCtx->oformat->flags & AVFMT_NOFILE)) {
    if (avio_open(&formatCtx->pb, outputFile.c_str(), AVIO_FLAG_WRITE) < 0) {
      std::cerr << "Error: Could not open output file: " << outputFile << std::endl;
      avformat_free_context(formatCtx);
      avcodec_free_context(&codecCtx);
      return false;
    }
  }

  // Write header
  if (avformat_write_header(formatCtx, nullptr) < 0) {
    std::cerr << "Error: Could not write format header" << std::endl;
    avio_closep(&formatCtx->pb);
    avformat_free_context(formatCtx);
    avcodec_free_context(&codecCtx);
    return false;
  }

  // Create frame and packet
  AVFrame* frame = av_frame_alloc();
  if (!frame) {
    std::cerr << "Error: Could not allocate frame" << std::endl;
    av_write_trailer(formatCtx);
    avio_closep(&formatCtx->pb);
    avformat_free_context(formatCtx);
    avcodec_free_context(&codecCtx);
    return false;
  }

  frame->format = codecCtx->pix_fmt;
  frame->width = codecCtx->width;
  frame->height = codecCtx->height;

  if (av_frame_get_buffer(frame, 0) < 0) {
    std::cerr << "Error: Could not allocate frame data" << std::endl;
    av_frame_free(&frame);
    av_write_trailer(formatCtx);
    avio_closep(&formatCtx->pb);
    avformat_free_context(formatCtx);
    avcodec_free_context(&codecCtx);
    return false;
  }

  // Create scaler for RGB to YUV420P conversion
  SwsContext* swsCtx = sws_getContext(imageWidth_, imageHeight_, AV_PIX_FMT_RGBA, codecCtx->width, codecCtx->height,
                                      codecCtx->pix_fmt, SWS_BILINEAR, nullptr, nullptr, nullptr);
  if (!swsCtx) {
    std::cerr << "Error: Could not create scaler context" << std::endl;
    av_frame_free(&frame);
    av_write_trailer(formatCtx);
    avio_closep(&formatCtx->pb);
    avformat_free_context(formatCtx);
    avcodec_free_context(&codecCtx);
    return false;
  }

  // Encode each frame
  AVPacket* pkt = av_packet_alloc();
  for (size_t i = 0; i < frameBuffer_.size(); ++i) {
    // Convert raylib Image to FFmpeg frame
    uint8_t* srcData[1] = {static_cast<uint8_t*>(frameBuffer_[i].data)};
    int srcLinesize[1] = {4 * imageWidth_};  // RGBA = 4 bytes per pixel

    sws_scale(swsCtx, srcData, srcLinesize, 0, imageHeight_, frame->data, frame->linesize);

    frame->pts = i;

    // Send frame to encoder
    if (avcodec_send_frame(codecCtx, frame) < 0) {
      std::cerr << "Error: Could not send frame " << i << " to encoder" << std::endl;
      continue;
    }

    // Receive encoded packets
    while (avcodec_receive_packet(codecCtx, pkt) == 0) {
      av_packet_rescale_ts(pkt, codecCtx->time_base, stream->time_base);
      pkt->stream_index = stream->index;
      av_interleaved_write_frame(formatCtx, pkt);
      av_packet_unref(pkt);
    }
  }

  // Flush encoder
  avcodec_send_frame(codecCtx, nullptr);
  while (avcodec_receive_packet(codecCtx, pkt) == 0) {
    av_packet_rescale_ts(pkt, codecCtx->time_base, stream->time_base);
    pkt->stream_index = stream->index;
    av_interleaved_write_frame(formatCtx, pkt);
    av_packet_unref(pkt);
  }

  // Write trailer and cleanup
  av_write_trailer(formatCtx);

  av_packet_free(&pkt);
  sws_freeContext(swsCtx);
  av_frame_free(&frame);
  avio_closep(&formatCtx->pb);
  avformat_free_context(formatCtx);
  avcodec_free_context(&codecCtx);

  std::cout << "Video saved: " << outputFile << std::endl;
  return true;
}

/**
 * @brief Factory function implementation for raylib backend.
 *
 * This replaces the CImg-based implementation as the default backend.
 */
std::unique_ptr<IRenderBackend> createDefaultRenderBackend() {
  return std::make_unique<RaylibRenderBackend>();
}

}  // namespace BioSim
