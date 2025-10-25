// test_basic_types.cpp
// Google Test version of unitTestBasicTypes
// This tests the types Dir, Coord, and Polar, and enum Compass.
// See basicTypes.h for more info about the basic types.

#include <gtest/gtest.h>
#include <cmath>
#include "basicTypes.h"

using namespace BioSim;

// Helper function to check floating point equality
bool areClosef(float a, float b) { 
  return std::abs(a - b) < 0.0001; 
}

// Test fixture for BasicTypes tests
class BasicTypesTest : public ::testing::Test {
protected:
  void SetUp() override {
    // Setup code if needed
  }

  void TearDown() override {
    // Cleanup code if needed
  }
};

// Dir tests - Constructor and basic operations
TEST_F(BasicTypesTest, DirConstructorFromCompass) {
  Dir d1 = Dir(Compass::N);
  Dir d2 = Dir(Compass::CENTER);
  d1 = d2;
  EXPECT_EQ(d1.asInt(), (int)Compass::CENTER);
}

TEST_F(BasicTypesTest, DirAsInt) {
  EXPECT_EQ(Dir(Compass::SW).asInt(), 0);
  EXPECT_EQ(Dir(Compass::S).asInt(), 1);
  EXPECT_EQ(Dir(Compass::SE).asInt(), 2);
  EXPECT_EQ(Dir(Compass::W).asInt(), 3);
  EXPECT_EQ(Dir(Compass::CENTER).asInt(), 4);
  EXPECT_EQ(Dir(Compass::E).asInt(), 5);
  EXPECT_EQ(Dir(Compass::NW).asInt(), 6);
  EXPECT_EQ(Dir(Compass::N).asInt(), 7);
  EXPECT_EQ(Dir(Compass::NE).asInt(), 8);
}

TEST_F(BasicTypesTest, DirCopyAssignment) {
  Dir d1 = Dir(Compass::N);
  Dir d2 = Compass::E;
  d1 = d2;
  EXPECT_EQ(d1.asInt(), 5);
  d2 = d1;
  EXPECT_EQ(d1.asInt(), 5);
}

TEST_F(BasicTypesTest, DirAssignmentFromCompass) {
  Dir d1 = Compass::SW;
  EXPECT_EQ(d1.asInt(), 0);
  d1 = Compass::SE;
  EXPECT_EQ(d1.asInt(), 2);
}

TEST_F(BasicTypesTest, DirEqualityWithCompass) {
  Dir d1 = Compass::CENTER;
  EXPECT_EQ(d1, Compass::CENTER);
  d1 = Compass::SE;
  EXPECT_EQ(d1, Compass::SE);
  EXPECT_EQ(Dir(Compass::W), Compass::W);
  EXPECT_NE(Dir(Compass::W), Compass::NW);
}

TEST_F(BasicTypesTest, DirEqualityWithDir) {
  Dir d1 = Compass::N;
  Dir d2 = Compass::N;
  EXPECT_EQ(d1, d2);
  EXPECT_EQ(d2, d1);
  d1 = Compass::NE;
  EXPECT_NE(d1, d2);
  EXPECT_NE(d2, d1);
}

TEST_F(BasicTypesTest, DirRotate) {
  Dir d1 = Compass::NE;
  EXPECT_EQ(d1.rotate(1), Compass::E);
  EXPECT_EQ(d1.rotate(2), Compass::SE);
  EXPECT_EQ(d1.rotate(-1), Compass::N);
  EXPECT_EQ(d1.rotate(-2), Compass::NW);
  EXPECT_EQ(Dir(Compass::N).rotate(1), d1);
  EXPECT_EQ(Dir(Compass::SW).rotate(-2), Compass::SE);
}

TEST_F(BasicTypesTest, DirAsNormalizedCoord) {
  Coordinate c1 = Dir(Compass::CENTER).asNormalizedCoord();
  EXPECT_EQ(c1.x, 0);
  EXPECT_EQ(c1.y, 0);
  
  Dir d1 = Compass::SW;
  c1 = d1.asNormalizedCoord();
  EXPECT_EQ(c1.x, -1);
  EXPECT_EQ(c1.y, -1);
  
  c1 = Dir(Compass::S).asNormalizedCoord();
  EXPECT_EQ(c1.x, 0);
  EXPECT_EQ(c1.y, -1);
  
  c1 = Dir(Compass::SE).asNormalizedCoord();
  EXPECT_EQ(c1.x, 1);
  EXPECT_EQ(c1.y, -1);
  
  c1 = Dir(Compass::W).asNormalizedCoord();
  EXPECT_EQ(c1.x, -1);
  EXPECT_EQ(c1.y, 0);
  
  c1 = Dir(Compass::E).asNormalizedCoord();
  EXPECT_EQ(c1.x, 1);
  EXPECT_EQ(c1.y, 0);
  
  c1 = Dir(Compass::NW).asNormalizedCoord();
  EXPECT_EQ(c1.x, -1);
  EXPECT_EQ(c1.y, 1);
  
  c1 = Dir(Compass::N).asNormalizedCoord();
  EXPECT_EQ(c1.x, 0);
  EXPECT_EQ(c1.y, 1);
  
  c1 = Dir(Compass::NE).asNormalizedCoord();
  EXPECT_EQ(c1.x, 1);
  EXPECT_EQ(c1.y, 1);
}

TEST_F(BasicTypesTest, DirAsNormalizedPolar) {
  Dir d1 = Compass::SW;
  Polar p1 = d1.asNormalizedPolar();
  EXPECT_EQ(p1.mag, 1);
  EXPECT_EQ(p1.dir, Compass::SW);
  
  p1 = Dir(Compass::S).asNormalizedPolar();
  EXPECT_EQ(p1.mag, 1);
  EXPECT_EQ(p1.dir, Compass::S);
  
  p1 = Dir(Compass::SE).asNormalizedPolar();
  EXPECT_EQ(p1.mag, 1);
  EXPECT_EQ(p1.dir, Compass::SE);
  
  p1 = Dir(Compass::W).asNormalizedPolar();
  EXPECT_EQ(p1.mag, 1);
  EXPECT_EQ(p1.dir, Compass::W);
  
  p1 = Dir(Compass::E).asNormalizedPolar();
  EXPECT_EQ(p1.mag, 1);
  EXPECT_EQ(p1.dir, Compass::E);
  
  p1 = Dir(Compass::NW).asNormalizedPolar();
  EXPECT_EQ(p1.mag, 1);
  EXPECT_EQ(p1.dir, Compass::NW);
  
  p1 = Dir(Compass::N).asNormalizedPolar();
  EXPECT_EQ(p1.mag, 1);
  EXPECT_EQ(p1.dir, Compass::N);
  
  p1 = Dir(Compass::NE).asNormalizedPolar();
  EXPECT_EQ(p1.mag, 1);
  EXPECT_EQ(p1.dir, Compass::NE);
}

// Coordinate tests
TEST_F(BasicTypesTest, CoordConstructor) {
  Coordinate c1 = Coordinate();
  EXPECT_EQ(c1.x, 0);
  EXPECT_EQ(c1.y, 0);
  
  c1 = Coordinate(1, 1);
  EXPECT_EQ(c1.x, 1);
  EXPECT_EQ(c1.y, 1);
  
  c1 = Coordinate(-6, 12);
  EXPECT_EQ(c1.x, -6);
  EXPECT_EQ(c1.y, 12);
}

TEST_F(BasicTypesTest, CoordCopyAssignment) {
  Coordinate c2 = Coordinate(9, 101);
  EXPECT_EQ(c2.x, 9);
  EXPECT_EQ(c2.y, 101);
  
  Coordinate c1 = c2;
  EXPECT_EQ(c1.x, 9);
  EXPECT_EQ(c2.y, 101);
}

TEST_F(BasicTypesTest, CoordIsNormalized) {
  EXPECT_FALSE(Coordinate(9, 101).isNormalized());
  EXPECT_TRUE(Coordinate(0, 0).isNormalized());
  EXPECT_TRUE(Coordinate(0, 1).isNormalized());
  EXPECT_TRUE(Coordinate(1, 1).isNormalized());
  EXPECT_TRUE(Coordinate(-1, 0).isNormalized());
  EXPECT_TRUE(Coordinate(-1, -1).isNormalized());
  EXPECT_FALSE(Coordinate(0, 2).isNormalized());
  EXPECT_FALSE(Coordinate(1, 2).isNormalized());
  EXPECT_FALSE(Coordinate(-1, 2).isNormalized());
  EXPECT_FALSE(Coordinate(-2, 0).isNormalized());
}

TEST_F(BasicTypesTest, CoordNormalize) {
  Coordinate c1 = Coordinate(0, 0);
  Coordinate c2 = c1.normalize();
  EXPECT_EQ(c2.x, 0);
  EXPECT_EQ(c2.y, 0);
  EXPECT_EQ(c2.asDir(), Compass::CENTER);
  
  c1 = Coordinate(0, 1).normalize();
  EXPECT_EQ(c1.x, 0);
  EXPECT_EQ(c1.y, 1);
  EXPECT_EQ(c1.asDir(), Compass::N);
  
  c1 = Coordinate(-1, 1).normalize();
  EXPECT_EQ(c1.x, -1);
  EXPECT_EQ(c1.y, 1);
  EXPECT_EQ(c1.asDir(), Compass::NW);
  
  c1 = Coordinate(100, 5).normalize();
  EXPECT_EQ(c1.x, 1);
  EXPECT_EQ(c1.y, 0);
  EXPECT_EQ(c1.asDir(), Compass::E);
  
  c1 = Coordinate(100, 105).normalize();
  EXPECT_EQ(c1.x, 1);
  EXPECT_EQ(c1.y, 1);
  EXPECT_EQ(c1.asDir(), Compass::NE);
  
  c1 = Coordinate(-5, 101).normalize();
  EXPECT_EQ(c1.x, 0);
  EXPECT_EQ(c1.y, 1);
  EXPECT_EQ(c1.asDir(), Compass::N);
  
  c1 = Coordinate(-500, 10).normalize();
  EXPECT_EQ(c1.x, -1);
  EXPECT_EQ(c1.y, 0);
  EXPECT_EQ(c1.asDir(), Compass::W);
  
  c1 = Coordinate(-500, -490).normalize();
  EXPECT_EQ(c1.x, -1);
  EXPECT_EQ(c1.y, -1);
  EXPECT_EQ(c1.asDir(), Compass::SW);
  
  c1 = Coordinate(-1, -490).normalize();
  EXPECT_EQ(c1.x, 0);
  EXPECT_EQ(c1.y, -1);
  EXPECT_EQ(c1.asDir(), Compass::S);
  
  c1 = Coordinate(1101, -1090).normalize();
  EXPECT_EQ(c1.x, 1);
  EXPECT_EQ(c1.y, -1);
  EXPECT_EQ(c1.asDir(), Compass::SE);
  
  c1 = Coordinate(1101, -3).normalize();
  EXPECT_EQ(c1.x, 1);
  EXPECT_EQ(c1.y, 0);
  EXPECT_EQ(c1.asDir(), Compass::E);
}

TEST_F(BasicTypesTest, CoordLength) {
  EXPECT_EQ(Coordinate(0, 0).length(), 0);
  EXPECT_EQ(Coordinate(0, 1).length(), 1);
  EXPECT_EQ(Coordinate(-1, 0).length(), 1);
  EXPECT_EQ(Coordinate(-1, -1).length(), 1);  // round down
  EXPECT_EQ(Coordinate(22, 0).length(), 22);
  EXPECT_EQ(Coordinate(22, 22).length(), 31);   // round down
  EXPECT_EQ(Coordinate(10, -10).length(), 14);  // round down
  EXPECT_EQ(Coordinate(-310, 0).length(), 310);
}

TEST_F(BasicTypesTest, CoordAsPolar) {
  Polar p1 = Coordinate(0, 0).asPolar();
  EXPECT_EQ(p1.mag, 0);
  EXPECT_EQ(p1.dir, Compass::CENTER);
  
  p1 = Coordinate(0, 1).asPolar();
  EXPECT_EQ(p1.mag, 1);
  EXPECT_EQ(p1.dir, Compass::N);
  
  p1 = Coordinate(-10, -10).asPolar();
  EXPECT_EQ(p1.mag, 14);
  EXPECT_EQ(p1.dir, Compass::SW);  // round down mag
  
  p1 = Coordinate(100, 1).asPolar();
  EXPECT_EQ(p1.mag, 100);
  EXPECT_EQ(p1.dir, Compass::E);  // round down mag
}

TEST_F(BasicTypesTest, CoordAdditionSubtraction) {
  Coordinate c1 = Coordinate(0, 0) + Coordinate(6, 8);
  EXPECT_EQ(c1.x, 6);
  EXPECT_EQ(c1.y, 8);
  
  c1 = Coordinate(-70, 20) + Coordinate(10, -10);
  EXPECT_EQ(c1.x, -60);
  EXPECT_EQ(c1.y, 10);
  
  c1 = Coordinate(-70, 20) - Coordinate(10, -10);
  EXPECT_EQ(c1.x, -80);
  EXPECT_EQ(c1.y, 30);
}

TEST_F(BasicTypesTest, CoordMultiplication) {
  Coordinate c1 = Coordinate(0, 0) * 1;
  EXPECT_EQ(c1.x, 0);
  EXPECT_EQ(c1.y, 0);
  
  c1 = Coordinate(1, 1) * -5;
  EXPECT_EQ(c1.x, -5);
  EXPECT_EQ(c1.y, -5);
  
  c1 = Coordinate(11, 5) * -5;
  EXPECT_EQ(c1.x, -55);
  EXPECT_EQ(c1.y, -25);
}

TEST_F(BasicTypesTest, CoordWithDir) {
  Coordinate c1 = Coordinate(0, 0);
  Coordinate c2 = c1 + Dir(Compass::CENTER);
  EXPECT_EQ(c2.x, 0);
  EXPECT_EQ(c2.y, 0);
  
  c2 = c1 + Dir(Compass::E);
  EXPECT_EQ(c2.x, 1);
  EXPECT_EQ(c2.y, 0);
  
  c2 = c1 + Dir(Compass::W);
  EXPECT_EQ(c2.x, -1);
  EXPECT_EQ(c2.y, 0);
  
  c2 = c1 + Dir(Compass::SW);
  EXPECT_EQ(c2.x, -1);
  EXPECT_EQ(c2.y, -1);

  c2 = c1 - Dir(Compass::CENTER);
  EXPECT_EQ(c2.x, 0);
  EXPECT_EQ(c2.y, 0);
  
  c2 = c1 - Dir(Compass::E);
  EXPECT_EQ(c2.x, -1);
  EXPECT_EQ(c2.y, 0);
  
  c2 = c1 - Dir(Compass::W);
  EXPECT_EQ(c2.x, 1);
  EXPECT_EQ(c2.y, 0);
  
  c2 = c1 - Dir(Compass::SW);
  EXPECT_EQ(c2.x, 1);
  EXPECT_EQ(c2.y, 1);
}

TEST_F(BasicTypesTest, CoordRaySameness) {
  Coordinate c1 = Coordinate{0, 0};
  Coordinate c2 = Coordinate{10, 11};
  Dir d1 = Compass::CENTER;
  
  EXPECT_FLOAT_EQ(c1.raySameness(c2), 1.0);  // special case - zero vector
  EXPECT_FLOAT_EQ(c2.raySameness(c1), 1.0);  // special case - zero vector
  EXPECT_FLOAT_EQ(c2.raySameness(d1), 1.0);  // special case - zero vector
  
  c1 = c2;
  EXPECT_FLOAT_EQ(c1.raySameness(c2), 1.0);
  
  EXPECT_TRUE(areClosef(Coordinate(-10, -10).raySameness(Coordinate(10, 10)), -1.0));
  
  c1 = Coordinate{0, 11};
  c2 = Coordinate{20, 0};
  EXPECT_TRUE(areClosef(c1.raySameness(c2), 0.0));
  EXPECT_TRUE(areClosef(c2.raySameness(c1), 0.0));
  
  c1 = Coordinate{0, 444};
  c2 = Coordinate{113, 113};
  EXPECT_TRUE(areClosef(c1.raySameness(c2), 0.707106781));
  
  c2 = Coordinate{113, -113};
  EXPECT_TRUE(areClosef(c1.raySameness(c2), -0.707106781));
}

// Polar tests
TEST_F(BasicTypesTest, PolarConstructor) {
  Polar p1 = Polar();
  EXPECT_EQ(p1.mag, 0);
  EXPECT_EQ(p1.dir, Compass::CENTER);
  
  p1 = Polar(0, Compass::S);
  EXPECT_EQ(p1.mag, 0);
  EXPECT_EQ(p1.dir, Compass::S);
  
  p1 = Polar(10, Compass::SE);
  EXPECT_EQ(p1.mag, 10);
  EXPECT_EQ(p1.dir, Compass::SE);
  
  p1 = Polar(-10, Compass::NW);
  EXPECT_EQ(p1.mag, -10);
  EXPECT_EQ(p1.dir, Compass::NW);
}

TEST_F(BasicTypesTest, PolarAsCoord) {
  Coordinate c1 = Polar(0, Compass::CENTER).asCoord();
  EXPECT_EQ(c1.x, 0);
  EXPECT_EQ(c1.y, 0);
  
  c1 = Polar(10, Compass::CENTER).asCoord();
  EXPECT_EQ(c1.x, 0);
  EXPECT_EQ(c1.y, 0);
  
  c1 = Polar(20, Compass::N).asCoord();
  EXPECT_EQ(c1.x, 0);
  EXPECT_EQ(c1.y, 20);
  
  Polar p1 = Polar(12, Compass::W);
  c1 = p1.asCoord();
  EXPECT_EQ(c1.x, -12);
  EXPECT_EQ(c1.y, 0);
  
  c1 = Polar(14, Compass::NE).asCoord();
  EXPECT_EQ(c1.x, 10);
  EXPECT_EQ(c1.y, 10);
  
  c1 = Polar(-14, Compass::NE).asCoord();
  EXPECT_EQ(c1.x, -10);
  EXPECT_EQ(c1.y, -10);
  
  c1 = Polar(14, Compass::E).asCoord();
  EXPECT_EQ(c1.x, 14);
  EXPECT_EQ(c1.y, 0);
  
  c1 = Polar(-14, Compass::E).asCoord();
  EXPECT_EQ(c1.x, -14);
  EXPECT_EQ(c1.y, 0);
}

// Main function for running tests
int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
