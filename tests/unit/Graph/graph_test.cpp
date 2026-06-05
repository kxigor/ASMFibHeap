#include <gtest/gtest.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <sys/stat.h>
#include <unistd.h>

extern "C" {
#include "FibHeap/FibHeap.h"
#include "Graph/Graph.h"
}

namespace {

// Reads a whole file into a string (empty string if it does not exist).
std::string slurp(const char* path) {
  std::ifstream in(path);
  std::stringstream ss;
  ss << in.rdbuf();
  return ss.str();
}

// Each test runs in its own scratch directory so the generated
// fib_heap.dot / fib_heap.png artifacts never collide or leak.
class GraphTest : public ::testing::Test {
 protected:
  void SetUp() override {
    ASSERT_NE(getcwd(prev_cwd_, sizeof(prev_cwd_)), nullptr);
    std::snprintf(scratch_, sizeof(scratch_), "graph_test_XXXXXX");
    ASSERT_NE(mkdtemp(scratch_), nullptr);
    ASSERT_EQ(chdir(scratch_), 0);
  }

  void TearDown() override {
    // Restore CWD before nuking the scratch directory.
    ASSERT_EQ(chdir(prev_cwd_), 0);
    std::string rm = "rm -rf '";
    rm += prev_cwd_;
    rm += "/";
    rm += scratch_;
    rm += "'";
    ASSERT_EQ(system(rm.c_str()), 0);
  }

  char prev_cwd_[4096] = {0};
  char scratch_[64] = {0};
};

// Build a consolidated heap (parent/child links) by inserting 0..n-1
// and extracting the min once, so generateDot recurses into children.
FibHeap* buildConsolidatedHeap(int n) {
  FibHeap* heap = fibHeapCtor();
  for (int i = 0; i < n; ++i) {
    fibHeapIns(heap, i);
  }
  fibHeapExtMin(heap);
  return heap;
}

TEST_F(GraphTest, EmptyHeapEmitsDigraphWithoutNodes) {
  FibHeap* heap = fibHeapCtor();
  generateFibHeapDot(heap);  // heap->min == NULL — skips generateDot
  fibHeapDtor(heap);

  std::string dot = slurp("fib_heap.dot");
  EXPECT_NE(dot.find("digraph G"), std::string::npos);
  EXPECT_NE(dot.find("ASM Fibonacci heap by KXI"), std::string::npos);
  // No node declarations for an empty heap.
  EXPECT_EQ(dot.find("key="), std::string::npos);
}

TEST_F(GraphTest, SingletonHeapEmitsExactlyOneRedMinNode) {
  FibHeap* heap = fibHeapInit(7);
  generateFibHeapDot(heap);
  fibHeapDtor(heap);

  std::string dot = slurp("fib_heap.dot");
  EXPECT_NE(dot.find("key=7"), std::string::npos);
  // The min node is colored red.
  EXPECT_NE(dot.find("[color=red]"), std::string::npos);
}

TEST_F(GraphTest, ConsolidatedHeapEmitsChildEdges) {
  FibHeap* heap = buildConsolidatedHeap(16);
  generateFibHeapDot(heap);
  fibHeapDtor(heap);

  std::string dot = slurp("fib_heap.dot");
  ASSERT_FALSE(dot.empty());
  // Parent→child edges are black; sibling edges blue/green. A consolidated
  // heap of 16 nodes always has at least one parent→child relationship.
  EXPECT_NE(dot.find("[color=black]"), std::string::npos);
  EXPECT_NE(dot.find("[color=blue]"), std::string::npos);
  EXPECT_NE(dot.find("[color=green]"), std::string::npos);
  EXPECT_NE(dot.find("[color=red]"), std::string::npos);

  // A rendered PNG should have been produced by the `dot` invocation.
  EXPECT_FALSE(slurp("fib_heap.png").empty());
}

TEST_F(GraphTest, UnwritableDirectoryIsHandledGracefully) {
  // Create a sub-directory with no write permission and run from inside it,
  // so fopen("fib_heap.dot", "w") fails and the error branch is taken.
  ASSERT_EQ(mkdir("ro", 0500), 0);
  ASSERT_EQ(chdir("ro"), 0);

  FibHeap* heap = fibHeapInit(1);
  generateFibHeapDot(heap);  // fopen fails → perror + early return, no crash
  fibHeapDtor(heap);

  EXPECT_TRUE(slurp("fib_heap.dot").empty());

  // Climb back out and restore permissions so TearDown can clean up.
  ASSERT_EQ(chdir(".."), 0);
  ASSERT_EQ(chmod("ro", 0700), 0);
}

}  // namespace
