/**
 * @file test_render_backend.cpp
 * @brief Unit tests for the IRenderBackend abstraction layer.
 *
 * These tests verify the rendering backend interface behavior and ensure
 * correct operation during the migration from CImg/OpenCV to raylib.
 * They use a mock implementation to test the interface contract without
 * depending on actual graphics libraries.
 */

#include "renderBackend.h"

#include <gtest/gtest.h>

#include <memory>
#include <vector>

namespace BioSim {

/**
 * @class MockRenderBackend
 * @brief Test double for IRenderBackend that records all method calls.
 *
 * This mock implementation allows us to test ImageWriter logic without
 * depending on CImg/OpenCV. It records all draw operations and allows
 * verification of correct call sequences and parameters.
 */
class MockRenderBackend : public IRenderBackend {
 public:
  struct DrawCall {
    enum Type { RECTANGLE, CIRCLE, CHALLENGE_ZONE } type;
    int16_t x1, y1, x2, y2;  // For rectangles
    int16_t centerX, centerY;
    uint16_t radius;
    Color color;
    ChallengeZoneType zoneType;
  };

  void init(uint16_t gridWidth, uint16_t gridHeight, uint16_t displayScale, uint16_t agentSize) override {
    gridWidth_ = gridWidth;
    gridHeight_ = gridHeight;
    displayScale_ = displayScale;
    agentSize_ = agentSize;
    initialized_ = true;
  }

  void startNewGeneration() override {
    frames_.clear();
    currentFrame_.clear();
    generationStarted_ = true;
  }

  void beginFrame(unsigned simStep, unsigned generation) override {
    currentFrame_.clear();
    frameInProgress_ = true;
    lastSimStep_ = simStep;
    lastGeneration_ = generation;
  }

  void drawChallengeZone(ChallengeZoneType zoneType, unsigned simStep, unsigned stepsPerGeneration) override {
    DrawCall call;
    call.type = DrawCall::CHALLENGE_ZONE;
    call.zoneType = zoneType;
    currentFrame_.push_back(call);
  }

  void drawRectangle(int16_t x1, int16_t y1, int16_t x2, int16_t y2, const Color& color) override {
    DrawCall call;
    call.type = DrawCall::RECTANGLE;
    call.x1 = x1;
    call.y1 = y1;
    call.x2 = x2;
    call.y2 = y2;
    call.color = color;
    currentFrame_.push_back(call);
  }

  void drawCircle(int16_t centerX, int16_t centerY, uint16_t radius, const Color& color) override {
    DrawCall call;
    call.type = DrawCall::CIRCLE;
    call.centerX = centerX;
    call.centerY = centerY;
    call.radius = radius;
    call.color = color;
    currentFrame_.push_back(call);
  }

  void endFrame() override {
    frames_.push_back(currentFrame_);
    currentFrame_.clear();
    frameInProgress_ = false;
  }

  bool saveVideo(unsigned generation, const std::string& outputPath) override {
    videoSaved_ = true;
    savedGeneration_ = generation;
    savedPath_ = outputPath;
    return !frames_.empty();
  }

  size_t getFrameCount() const override { return frames_.size(); }

  // Test accessors
  bool isInitialized() const { return initialized_; }
  bool isGenerationStarted() const { return generationStarted_; }
  bool isFrameInProgress() const { return frameInProgress_; }
  const std::vector<std::vector<DrawCall>>& getFrames() const { return frames_; }
  const std::vector<DrawCall>& getCurrentFrame() const { return currentFrame_; }
  unsigned getLastSimStep() const { return lastSimStep_; }
  unsigned getLastGeneration() const { return lastGeneration_; }
  bool wasVideoSaved() const { return videoSaved_; }
  unsigned getSavedGeneration() const { return savedGeneration_; }
  const std::string& getSavedPath() const { return savedPath_; }

 private:
  bool initialized_ = false;
  bool generationStarted_ = false;
  bool frameInProgress_ = false;
  bool videoSaved_ = false;
  unsigned lastSimStep_ = 0;
  unsigned lastGeneration_ = 0;
  unsigned savedGeneration_ = 0;
  std::string savedPath_;
  uint16_t gridWidth_ = 0;
  uint16_t gridHeight_ = 0;
  uint16_t displayScale_ = 1;
  uint16_t agentSize_ = 1;
  std::vector<DrawCall> currentFrame_;
  std::vector<std::vector<DrawCall>> frames_;
};

// ========== Color Tests ==========

TEST(ColorTest, DefaultConstructor) {
  Color c;
  EXPECT_EQ(c.r, 0);
  EXPECT_EQ(c.g, 0);
  EXPECT_EQ(c.b, 0);
  EXPECT_EQ(c.a, 255);  // Default opaque
}

TEST(ColorTest, RGBConstructor) {
  Color c(128, 64, 192);
  EXPECT_EQ(c.r, 128);
  EXPECT_EQ(c.g, 64);
  EXPECT_EQ(c.b, 192);
  EXPECT_EQ(c.a, 255);  // Default alpha
}

TEST(ColorTest, RGBAConstructor) {
  Color c(255, 128, 64, 32);
  EXPECT_EQ(c.r, 255);
  EXPECT_EQ(c.g, 128);
  EXPECT_EQ(c.b, 64);
  EXPECT_EQ(c.a, 32);
}

TEST(ColorTest, FromFloatFullyOpaque) {
  Color c = Color::fromFloat(1.0f, 0.5f, 0.25f, 1.0f);
  EXPECT_EQ(c.r, 255);
  EXPECT_EQ(c.g, 127);  // 0.5 * 255 = 127.5 → 127
  EXPECT_EQ(c.b, 63);   // 0.25 * 255 = 63.75 → 63
  EXPECT_EQ(c.a, 255);
}

TEST(ColorTest, FromFloatTransparent) {
  Color c = Color::fromFloat(0.5f, 0.5f, 0.5f, 0.5f);
  EXPECT_EQ(c.r, 127);
  EXPECT_EQ(c.g, 127);
  EXPECT_EQ(c.b, 127);
  EXPECT_EQ(c.a, 127);
}

TEST(ColorTest, FromFloatBoundaries) {
  Color black = Color::fromFloat(0.0f, 0.0f, 0.0f, 0.0f);
  EXPECT_EQ(black.r, 0);
  EXPECT_EQ(black.g, 0);
  EXPECT_EQ(black.b, 0);
  EXPECT_EQ(black.a, 0);

  Color white = Color::fromFloat(1.0f, 1.0f, 1.0f, 1.0f);
  EXPECT_EQ(white.r, 255);
  EXPECT_EQ(white.g, 255);
  EXPECT_EQ(white.b, 255);
  EXPECT_EQ(white.a, 255);
}

// ========== Backend Interface Tests ==========

TEST(MockRenderBackendTest, InitializationState) {
  MockRenderBackend backend;
  EXPECT_FALSE(backend.isInitialized());

  backend.init(128, 128, 4, 2);
  EXPECT_TRUE(backend.isInitialized());
}

TEST(MockRenderBackendTest, GenerationLifecycle) {
  MockRenderBackend backend;
  backend.init(128, 128, 4, 2);

  EXPECT_FALSE(backend.isGenerationStarted());
  backend.startNewGeneration();
  EXPECT_TRUE(backend.isGenerationStarted());
  EXPECT_EQ(backend.getFrameCount(), 0);
}

TEST(MockRenderBackendTest, FrameLifecycle) {
  MockRenderBackend backend;
  backend.init(128, 128, 4, 2);
  backend.startNewGeneration();

  EXPECT_FALSE(backend.isFrameInProgress());

  backend.beginFrame(0, 0);
  EXPECT_TRUE(backend.isFrameInProgress());
  EXPECT_EQ(backend.getLastSimStep(), 0);
  EXPECT_EQ(backend.getLastGeneration(), 0);

  backend.endFrame();
  EXPECT_FALSE(backend.isFrameInProgress());
  EXPECT_EQ(backend.getFrameCount(), 1);
}

TEST(MockRenderBackendTest, MultipleFrames) {
  MockRenderBackend backend;
  backend.init(128, 128, 4, 2);
  backend.startNewGeneration();

  // Capture 3 frames
  for (unsigned i = 0; i < 3; ++i) {
    backend.beginFrame(i, 0);
    backend.endFrame();
  }

  EXPECT_EQ(backend.getFrameCount(), 3);

  // New generation should clear frames
  backend.startNewGeneration();
  EXPECT_EQ(backend.getFrameCount(), 0);
}

TEST(MockRenderBackendTest, DrawRectangle) {
  MockRenderBackend backend;
  backend.init(128, 128, 4, 2);
  backend.startNewGeneration();
  backend.beginFrame(0, 0);

  Color blue(0, 0, 255, 128);
  backend.drawRectangle(10, 20, 30, 40, blue);

  const auto& frame = backend.getCurrentFrame();
  ASSERT_EQ(frame.size(), 1);
  EXPECT_EQ(frame[0].type, MockRenderBackend::DrawCall::RECTANGLE);
  EXPECT_EQ(frame[0].x1, 10);
  EXPECT_EQ(frame[0].y1, 20);
  EXPECT_EQ(frame[0].x2, 30);
  EXPECT_EQ(frame[0].y2, 40);
  EXPECT_EQ(frame[0].color.r, 0);
  EXPECT_EQ(frame[0].color.g, 0);
  EXPECT_EQ(frame[0].color.b, 255);
  EXPECT_EQ(frame[0].color.a, 128);
}

TEST(MockRenderBackendTest, DrawCircle) {
  MockRenderBackend backend;
  backend.init(128, 128, 4, 2);
  backend.startNewGeneration();
  backend.beginFrame(0, 0);

  Color red(255, 0, 0, 255);
  backend.drawCircle(64, 64, 5, red);

  const auto& frame = backend.getCurrentFrame();
  ASSERT_EQ(frame.size(), 1);
  EXPECT_EQ(frame[0].type, MockRenderBackend::DrawCall::CIRCLE);
  EXPECT_EQ(frame[0].centerX, 64);
  EXPECT_EQ(frame[0].centerY, 64);
  EXPECT_EQ(frame[0].radius, 5);
  EXPECT_EQ(frame[0].color.r, 255);
  EXPECT_EQ(frame[0].color.g, 0);
  EXPECT_EQ(frame[0].color.b, 0);
  EXPECT_EQ(frame[0].color.a, 255);
}

TEST(MockRenderBackendTest, DrawChallengeZone) {
  MockRenderBackend backend;
  backend.init(128, 128, 4, 2);
  backend.startNewGeneration();
  backend.beginFrame(0, 0);

  backend.drawChallengeZone(ChallengeZoneType::CENTER_WEIGHTED, 50, 300);

  const auto& frame = backend.getCurrentFrame();
  ASSERT_EQ(frame.size(), 1);
  EXPECT_EQ(frame[0].type, MockRenderBackend::DrawCall::CHALLENGE_ZONE);
  EXPECT_EQ(frame[0].zoneType, ChallengeZoneType::CENTER_WEIGHTED);
}

TEST(MockRenderBackendTest, ComplexFrame) {
  MockRenderBackend backend;
  backend.init(128, 128, 4, 2);
  backend.startNewGeneration();
  backend.beginFrame(0, 0);

  // Draw multiple elements
  backend.drawChallengeZone(ChallengeZoneType::RADIOACTIVE_WALLS, 100, 300);
  backend.drawRectangle(0, 0, 10, 10, Color(255, 0, 0));
  backend.drawCircle(50, 50, 3, Color(0, 255, 0));
  backend.drawCircle(60, 60, 3, Color(0, 0, 255));

  const auto& frame = backend.getCurrentFrame();
  EXPECT_EQ(frame.size(), 4);

  backend.endFrame();

  const auto& frames = backend.getFrames();
  ASSERT_EQ(frames.size(), 1);
  EXPECT_EQ(frames[0].size(), 4);
}

TEST(MockRenderBackendTest, SaveVideo) {
  MockRenderBackend backend;
  backend.init(128, 128, 4, 2);
  backend.startNewGeneration();

  // Create 2 frames
  backend.beginFrame(0, 5);
  backend.drawCircle(10, 10, 2, Color(255, 0, 0));
  backend.endFrame();

  backend.beginFrame(1, 5);
  backend.drawCircle(20, 20, 2, Color(0, 255, 0));
  backend.endFrame();

  EXPECT_FALSE(backend.wasVideoSaved());

  bool success = backend.saveVideo(5, "output/images");
  EXPECT_TRUE(success);
  EXPECT_TRUE(backend.wasVideoSaved());
  EXPECT_EQ(backend.getSavedGeneration(), 5);
  EXPECT_EQ(backend.getSavedPath(), "output/images");
}

TEST(MockRenderBackendTest, SaveEmptyVideoFails) {
  MockRenderBackend backend;
  backend.init(128, 128, 4, 2);
  backend.startNewGeneration();

  // No frames captured
  bool success = backend.saveVideo(0, "output/images");
  EXPECT_FALSE(success);  // Should fail with no frames
}

// ========== ChallengeZoneType Tests ==========

TEST(ChallengeZoneTypeTest, EnumValues) {
  // Verify enum values match expectations
  EXPECT_EQ(static_cast<int>(ChallengeZoneType::NONE), 0);
  EXPECT_EQ(static_cast<int>(ChallengeZoneType::CENTER_WEIGHTED), 1);
  EXPECT_EQ(static_cast<int>(ChallengeZoneType::CENTER_UNWEIGHTED), 2);
  EXPECT_EQ(static_cast<int>(ChallengeZoneType::RADIOACTIVE_WALLS), 3);
}

}  // namespace BioSim
