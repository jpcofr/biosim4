/// test_grid_visit_neighborhood.cpp
/// Google Test version of unitTestGridVisitNeighborhood

#include "../simulation/simulator.h"

#include <gtest/gtest.h>

#include <sstream>
#include <vector>

using namespace BioSim;

/// Test fixture for Grid Visit Neighborhood tests
class GridVisitNeighborhoodTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Initialize global params for testing (sets grid size to 128x128 by default)
    initParamsForTesting();
  }

  void TearDown() override {
    /// Cleanup code if needed
  }

  /// Helper to collect visited coordinates
  std::vector<Coordinate> collectVisitedCoords(Coordinate loc, float radius) {
    std::vector<Coordinate> visited;
    auto collector = [&visited](Coordinate c) { visited.push_back(c); };
    visitNeighborhood(loc, radius, collector);
    return visited;
  }
};

TEST_F(GridVisitNeighborhoodTest, VisitNeighborhoodRadius1_Center) {
  auto visited = collectVisitedCoords(Coordinate{10, 10}, 1.0);

  /// With radius 1.0, should visit 9 cells (3x3 grid centered on location)
  EXPECT_FALSE(visited.empty()) << "Should visit at least some cells";

  /// Verify the center location is included
  bool foundCenter = false;
  for (const auto& c : visited) {
    if (c.x == 10 && c.y == 10) {
      foundCenter = true;
      break;
    }
  }
  EXPECT_TRUE(foundCenter) << "Center location should be visited";
}

TEST_F(GridVisitNeighborhoodTest, VisitNeighborhoodRadius1_Corner) {
  auto visited = collectVisitedCoords(Coordinate{0, 0}, 1.0);

  /// At corner (0,0), should still visit neighborhood
  EXPECT_FALSE(visited.empty()) << "Should visit cells even at corner";

  /// All coordinates should be non-negative or handle boundaries properly
  for (const auto& c : visited) {
    EXPECT_GE(c.x, 0) << "X coordinate should be >= 0";
    EXPECT_GE(c.y, 0) << "Y coordinate should be >= 0";
  }
}

TEST_F(GridVisitNeighborhoodTest, VisitNeighborhoodRadius1_4) {
  auto visited1_0 = collectVisitedCoords(Coordinate{10, 10}, 1.0);
  auto visited1_4 = collectVisitedCoords(Coordinate{10, 10}, 1.4);

  /// Radius 1.4 should visit at least as many cells as radius 1.0
  EXPECT_GE(visited1_4.size(), visited1_0.size()) << "Larger radius should visit same or more cells";
}

TEST_F(GridVisitNeighborhoodTest, VisitNeighborhoodRadius1_5) {
  auto visited = collectVisitedCoords(Coordinate{10, 10}, 1.5);

  EXPECT_FALSE(visited.empty()) << "Should visit cells with radius 1.5";
}

TEST_F(GridVisitNeighborhoodTest, VisitNeighborhoodRadius2) {
  auto visited1 = collectVisitedCoords(Coordinate{10, 10}, 1.0);
  auto visited2 = collectVisitedCoords(Coordinate{10, 10}, 2.0);

  /// Radius 2.0 should visit more cells than radius 1.0
  EXPECT_GT(visited2.size(), visited1.size()) << "Radius 2.0 should visit more cells than radius 1.0";
}

TEST_F(GridVisitNeighborhoodTest, VisitNeighborhoodEdgeCase) {
  /// Test at grid boundary
  Coordinate edge{(int16_t)(parameterMngrSingleton.gridSize_X - 1), (int16_t)(parameterMngrSingleton.gridSize_Y - 1)};

  auto visited = collectVisitedCoords(edge, 2.0);

  EXPECT_FALSE(visited.empty()) << "Should visit cells even at grid edge";

  /// Verify coordinates are within bounds
  for (const auto& c : visited) {
    EXPECT_LT(c.x, parameterMngrSingleton.gridSize_X) << "X coordinate should be within grid bounds";
    EXPECT_LT(c.y, parameterMngrSingleton.gridSize_Y) << "Y coordinate should be within grid bounds";
  }
}

TEST_F(GridVisitNeighborhoodTest, VisitNeighborhoodPrintsCorrectly) {
  /// This test captures output to verify the function works without crashing
  std::stringstream ss;

  auto printLoc = [&ss](Coordinate loc) { ss << loc.x << ", " << loc.y << std::endl; };

  EXPECT_NO_THROW({ visitNeighborhood(Coordinate{10, 10}, 1.0, printLoc); })
      << "visitNeighborhood should not throw exceptions";

  std::string output = ss.str();
  EXPECT_FALSE(output.empty()) << "Should generate output for visited locations";
}

/// Main function for running tests
int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
