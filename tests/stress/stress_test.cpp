// Stress suite: high-volume workloads that push the heap deep into its
// consolidate / cascading-cut machinery, run identically against the C and
// the NASM implementation. These are correctness-at-scale checks, not
// micro-benchmarks — every run still validates output against a sorted model.
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <numeric>
#include <random>
#include <vector>

extern "C" {
#include "ASM_FibHeap/ASM_FibHeap.h"
#include "FibHeap/FibHeap.h"
}

namespace {

constexpr int kBig = 50000;

// ---- C implementation: bulk insert then drain, must come out sorted. ----
TEST(Stress, CHeapSortsLargeShuffledInput) {
  std::vector<Key_t> keys(kBig);
  std::iota(keys.begin(), keys.end(), 0);
  std::shuffle(keys.begin(), keys.end(), std::mt19937(42));

  FibHeap* heap = fibHeapCtor();
  for (Key_t k : keys) fibHeapIns(heap, k);
  ASSERT_EQ(fibHeapGetSize(heap), static_cast<uint64_t>(kBig));

  for (Key_t expected = 0; expected < kBig; ++expected) {
    ASSERT_EQ(fibHeapGetMin(heap)->key, expected);
    fibHeapExtMin(heap);
  }
  EXPECT_EQ(fibHeapGetSize(heap), 0u);
  fibHeapDtor(heap);
}

// ---- ASM implementation: same workload. ----
TEST(Stress, AsmHeapSortsLargeShuffledInput) {
  std::vector<Key_t> keys(kBig);
  std::iota(keys.begin(), keys.end(), 0);
  std::shuffle(keys.begin(), keys.end(), std::mt19937(42));

  ASM_FibHeap* heap = ASM_fibHeapCtor();
  for (Key_t k : keys) ASM_fibHeapIns(heap, k);
  ASSERT_EQ(ASM_fibHeapGetSize(heap), static_cast<uint64_t>(kBig));

  for (Key_t expected = 0; expected < kBig; ++expected) {
    ASSERT_EQ(ASM_fibHeapGetMin(heap)->key, expected);
    ASM_fibHeapExtMin(heap);
  }
  EXPECT_EQ(ASM_fibHeapGetSize(heap), 0u);
  ASM_fibHeapDtor(heap);
}

// ---- Heavy decrease-key load: forces long cascading-cut chains. ----
template <typename HeapT, typename NodeT>
struct Ops;

template <>
struct Ops<FibHeap, FibNode> {
  static FibHeap* ctor() { return fibHeapCtor(); }
  static FibNode* ins(FibHeap* h, Key_t k) { return fibHeapIns(h, k); }
  static void ext(FibHeap* h) { fibHeapExtMin(h); }
  static FibNode* min(FibHeap* h) { return fibHeapGetMin(h); }
  static void dec(FibHeap* h, FibNode* n, Key_t k) { fibHeapOverrideKey(h, n, k); }
  static uint64_t size(FibHeap* h) { return fibHeapGetSize(h); }
  static void dtor(FibHeap* h) { fibHeapDtor(h); }
};

template <>
struct Ops<ASM_FibHeap, ASM_FibNode> {
  static ASM_FibHeap* ctor() { return ASM_fibHeapCtor(); }
  static ASM_FibNode* ins(ASM_FibHeap* h, Key_t k) { return ASM_fibHeapIns(h, k); }
  static void ext(ASM_FibHeap* h) { ASM_fibHeapExtMin(h); }
  static ASM_FibNode* min(ASM_FibHeap* h) { return ASM_fibHeapGetMin(h); }
  static void dec(ASM_FibHeap* h, ASM_FibNode* n, Key_t k) {
    ASM_fibHeapOverrideKey(h, n, k);
  }
  static uint64_t size(ASM_FibHeap* h) { return ASM_fibHeapGetSize(h); }
  static void dtor(ASM_FibHeap* h) { ASM_fibHeapDtor(h); }
};

struct StormResult {
  bool storm_min_always_correct = true;  // each decrease produced the new min
  bool drain_sorted = true;              // extraction order was non-decreasing
  uint64_t final_size = 0;
};

// Runs the decrease-key storm and returns observations; assertions are made
// by the caller so no gtest macros live in this (templated) body.
template <typename HeapT, typename NodeT>
StormResult RunDecreaseKeyStorm() {
  constexpr int n = 20000;
  using O = Ops<HeapT, NodeT>;
  StormResult res;

  HeapT* heap = O::ctor();
  std::vector<NodeT*> nodes;
  nodes.reserve(n);

  // Insert ascending keys (spaced out so we have room to decrease).
  for (int i = 0; i < n; ++i) {
    nodes.push_back(O::ins(heap, static_cast<Key_t>(i) * 4));
  }
  // Consolidate by extracting once.
  O::ext(heap);
  nodes.erase(nodes.begin());

  // Decrease many keys to ever-smaller values, each becoming the new min.
  std::mt19937 rng(2024);
  std::shuffle(nodes.begin(), nodes.end(), rng);
  Key_t floor = -1;
  for (NodeT* node : nodes) {
    O::dec(heap, node, floor);
    if (O::min(heap)->key != floor) res.storm_min_always_correct = false;
    --floor;
  }

  // Drain — keys must emerge in ascending (most-negative-first) order.
  Key_t prev = INT64_MIN;
  while (O::size(heap) > 0) {
    Key_t cur = O::min(heap)->key;
    if (cur < prev) res.drain_sorted = false;
    prev = cur;
    O::ext(heap);
  }
  res.final_size = O::size(heap);
  O::dtor(heap);
  return res;
}

TEST(Stress, CHeapSurvivesDecreaseKeyStorm) {
  StormResult r = RunDecreaseKeyStorm<FibHeap, FibNode>();
  EXPECT_TRUE(r.storm_min_always_correct);
  EXPECT_TRUE(r.drain_sorted);
  EXPECT_EQ(r.final_size, 0u);
}

TEST(Stress, AsmHeapSurvivesDecreaseKeyStorm) {
  StormResult r = RunDecreaseKeyStorm<ASM_FibHeap, ASM_FibNode>();
  EXPECT_TRUE(r.storm_min_always_correct);
  EXPECT_TRUE(r.drain_sorted);
  EXPECT_EQ(r.final_size, 0u);
}

}  // namespace
