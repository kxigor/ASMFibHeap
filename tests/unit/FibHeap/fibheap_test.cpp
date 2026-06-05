#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <random>
#include <set>
#include <vector>

extern "C" {
#include "FibHeap/FibHeap.h"
}

namespace {

// =====================================================================
// Construction / size / get-min on an empty heap
// =====================================================================

TEST(FibHeap, CtorMakesEmptyHeap) {
  FibHeap* heap = fibHeapCtor();
  ASSERT_NE(heap, nullptr);
  EXPECT_EQ(fibHeapGetSize(heap), 0u);
  EXPECT_EQ(fibHeapGetMin(heap), nullptr);
  fibHeapDtor(heap);
}

TEST(FibHeap, InitMakesSingletonHeap) {
  FibHeap* heap = fibHeapInit(42);
  ASSERT_NE(heap, nullptr);
  EXPECT_EQ(fibHeapGetSize(heap), 1u);
  ASSERT_NE(fibHeapGetMin(heap), nullptr);
  EXPECT_EQ(fibHeapGetMin(heap)->key, 42);
  fibHeapDtor(heap);
}

TEST(FibHeap, DtorOnEmptyHeapDoesNotCrash) {
  FibHeap* heap = fibHeapCtor();
  fibHeapDtor(heap);  // min == NULL path inside fibNodeDtor
  SUCCEED();
}

// =====================================================================
// Insert keeps the minimum up to date (both branches of fibHeapIns)
// =====================================================================

TEST(FibHeap, InsertIntoEmptyHeapSetsMin) {
  FibHeap* heap = fibHeapCtor();
  FibNode* n = fibHeapIns(heap, 7);  // size == 0 branch
  ASSERT_NE(n, nullptr);
  EXPECT_EQ(fibHeapGetMin(heap), n);
  EXPECT_EQ(fibHeapGetSize(heap), 1u);
  fibHeapDtor(heap);
}

TEST(FibHeap, InsertSmallerKeyBecomesNewMin) {
  FibHeap* heap = fibHeapCtor();
  fibHeapIns(heap, 10);
  fibHeapIns(heap, 20);          // larger — min unchanged
  EXPECT_EQ(fibHeapGetMin(heap)->key, 10);
  FibNode* small = fibHeapIns(heap, 5);  // smaller — min updates
  EXPECT_EQ(fibHeapGetMin(heap), small);
  EXPECT_EQ(fibHeapGetSize(heap), 3u);
  fibHeapDtor(heap);
}

// =====================================================================
// Extract-min edge cases
// =====================================================================

TEST(FibHeap, ExtractMinOnEmptyIsNoOp) {
  FibHeap* heap = fibHeapCtor();
  fibHeapExtMin(heap);  // size == 0 early return
  EXPECT_EQ(fibHeapGetSize(heap), 0u);
  EXPECT_EQ(fibHeapGetMin(heap), nullptr);
  fibHeapDtor(heap);
}

TEST(FibHeap, ExtractMinOnSingletonEmptiesHeap) {
  FibHeap* heap = fibHeapInit(1);
  fibHeapExtMin(heap);  // size == 1 branch
  EXPECT_EQ(fibHeapGetSize(heap), 0u);
  EXPECT_EQ(fibHeapGetMin(heap), nullptr);
  fibHeapDtor(heap);
}

TEST(FibHeap, ExtractMinReturnsKeysInSortedOrder) {
  FibHeap* heap = fibHeapCtor();
  std::vector<Key_t> keys{5, 3, 8, 1, 9, 2, 7, 4, 6, 0};
  for (Key_t k : keys) {
    fibHeapIns(heap, k);
  }
  std::vector<Key_t> sorted = keys;
  std::sort(sorted.begin(), sorted.end());

  for (Key_t expected : sorted) {
    ASSERT_NE(fibHeapGetMin(heap), nullptr);
    EXPECT_EQ(fibHeapGetMin(heap)->key, expected);
    fibHeapExtMin(heap);  // exercises consolidate repeatedly
  }
  EXPECT_EQ(fibHeapGetSize(heap), 0u);
  fibHeapDtor(heap);
}

// =====================================================================
// Merge: empty-first, empty-second, and both-populated branches
// =====================================================================

TEST(FibHeap, MergeIntoEmptyFirstAdoptsSecond) {
  FibHeap* first = fibHeapCtor();  // size == 0 branch
  FibHeap* second = fibHeapCtor();
  fibHeapIns(second, 3);
  fibHeapIns(second, 1);
  fibHeapMerge(first, second);
  EXPECT_EQ(fibHeapGetSize(first), 2u);
  EXPECT_EQ(fibHeapGetMin(first)->key, 1);
  fibHeapDtor(first);
}

TEST(FibHeap, MergeWithEmptySecondLeavesFirstUnchanged) {
  FibHeap* first = fibHeapCtor();
  fibHeapIns(first, 4);
  fibHeapIns(first, 2);
  FibHeap* second = fibHeapCtor();  // size == 0 branch
  fibHeapMerge(first, second);
  EXPECT_EQ(fibHeapGetSize(first), 2u);
  EXPECT_EQ(fibHeapGetMin(first)->key, 2);
  fibHeapDtor(first);
}

TEST(FibHeap, MergeTwoPopulatedHeapsPicksGlobalMin) {
  FibHeap* first = fibHeapCtor();
  fibHeapIns(first, 10);
  fibHeapIns(first, 30);
  FibHeap* second = fibHeapCtor();
  fibHeapIns(second, 5);   // global min lives in the second heap
  fibHeapIns(second, 20);
  fibHeapMerge(first, second);
  EXPECT_EQ(fibHeapGetSize(first), 4u);
  EXPECT_EQ(fibHeapGetMin(first)->key, 5);

  // first heap keeps its own (smaller) min — exercises the other branch.
  FibHeap* a = fibHeapCtor();
  fibHeapIns(a, 1);
  fibHeapIns(a, 40);
  FibHeap* b = fibHeapCtor();
  fibHeapIns(b, 50);
  fibHeapMerge(a, b);
  EXPECT_EQ(fibHeapGetMin(a)->key, 1);

  fibHeapDtor(first);
  fibHeapDtor(a);
}

// =====================================================================
// Override-key (decrease-key) and arbitrary delete — drives cut and
// cascading-cut. We first consolidate into deep trees, then cut.
// =====================================================================

// Builds a heap whose nodes have been consolidated into multi-level
// trees by inserting 0..n-1 and extracting the min once.
static FibHeap* buildConsolidatedHeap(std::vector<FibNode*>& nodes, int n) {
  FibHeap* heap = fibHeapCtor();
  nodes.clear();
  for (int i = 0; i < n; ++i) {
    nodes.push_back(fibHeapIns(heap, i));
  }
  fibHeapExtMin(heap);  // forces a consolidate, building binomial trees
  nodes.erase(nodes.begin());  // key 0 was the extracted min
  return heap;
}

TEST(FibHeap, OverrideKeyToNewGlobalMin) {
  std::vector<FibNode*> nodes;
  FibHeap* heap = buildConsolidatedHeap(nodes, 32);
  // Pick a deep node and decrease it below everything else.
  FibNode* target = nodes.back();
  fibHeapOverrideKey(heap, target, -100);
  EXPECT_EQ(fibHeapGetMin(heap), target);
  EXPECT_EQ(fibHeapGetMin(heap)->key, -100);
  fibHeapDtor(heap);
}

TEST(FibHeap, OverrideKeyWithoutHeapViolationKeepsStructure) {
  std::vector<FibNode*> nodes;
  FibHeap* heap = buildConsolidatedHeap(nodes, 32);
  uint64_t size_before = fibHeapGetSize(heap);
  // Decrease a node only slightly — but not below its parent, so no cut.
  // node with key 31 down to 30 (still larger than many parents).
  fibHeapOverrideKey(heap, nodes.back(), nodes.back()->key);  // no-op-ish
  EXPECT_EQ(fibHeapGetSize(heap), size_before);
  fibHeapDtor(heap);
}

TEST(FibHeap, DeleteArbitraryNodesEmptiesHeapInOrder) {
  std::vector<FibNode*> nodes;
  FibHeap* heap = buildConsolidatedHeap(nodes, 16);

  // Delete every other node, then drain the rest by extract-min.
  for (size_t i = 0; i < nodes.size(); i += 2) {
    fibHeapDel(heap, nodes[i]);
  }
  // Remaining must still come out sorted and the heap must empty cleanly.
  Key_t prev = INT64_MIN;
  while (fibHeapGetSize(heap) > 0) {
    Key_t cur = fibHeapGetMin(heap)->key;
    EXPECT_GE(cur, prev);
    prev = cur;
    fibHeapExtMin(heap);
  }
  fibHeapDtor(heap);
}

TEST(FibHeap, CascadingCutThroughMarkedAncestors) {
  // A larger consolidated heap plus a batch of decrease-keys is the
  // reliable way to drive marks up several levels (cascading cut).
  std::vector<FibNode*> nodes;
  FibHeap* heap = buildConsolidatedHeap(nodes, 64);

  std::mt19937 rng(12345);
  std::shuffle(nodes.begin(), nodes.end(), rng);

  // Repeatedly decrease keys of deep nodes to force cuts + cascades.
  Key_t floor = -1;
  for (FibNode* n : nodes) {
    fibHeapOverrideKey(heap, n, floor);
    --floor;  // each new key is the new global minimum
    EXPECT_EQ(fibHeapGetMin(heap), n);
  }
  EXPECT_EQ(fibHeapGetSize(heap), nodes.size());
  fibHeapDtor(heap);
}

// =====================================================================
// Reference-model fuzz test: exercises consolidate / cut / cascading-cut
// across many randomized operation mixes and verifies correctness.
// =====================================================================

class FibHeapModelTest : public ::testing::TestWithParam<unsigned> {};

TEST_P(FibHeapModelTest, MatchesMultisetModel) {
  std::mt19937 rng(GetParam());
  FibHeap* heap = fibHeapCtor();

  std::multiset<Key_t> model;
  // live nodes still in the heap, paired with their current key.
  std::vector<std::pair<FibNode*, Key_t>> live;

  auto check_min = [&]() {
    if (model.empty()) {
      EXPECT_EQ(fibHeapGetMin(heap), nullptr);
    } else {
      ASSERT_NE(fibHeapGetMin(heap), nullptr);
      EXPECT_EQ(fibHeapGetMin(heap)->key, *model.begin());
    }
    EXPECT_EQ(fibHeapGetSize(heap), model.size());
  };

  std::uniform_int_distribution<int> op_dist(0, 99);
  std::uniform_int_distribution<Key_t> key_dist(-1000, 1000);

  for (int step = 0; step < 4000; ++step) {
    int op = op_dist(rng);

    if (op < 45 || live.empty()) {
      // Insert
      Key_t k = key_dist(rng);
      FibNode* n = fibHeapIns(heap, k);
      live.emplace_back(n, k);
      model.insert(k);
    } else if (op < 70) {
      // Extract-min — record the actual min node to drop from `live`.
      FibNode* m = fibHeapGetMin(heap);
      model.erase(model.begin());
      fibHeapExtMin(heap);
      auto it = std::find_if(live.begin(), live.end(),
                             [&](auto& p) { return p.first == m; });
      ASSERT_NE(it, live.end());
      live.erase(it);
    } else if (op < 85) {
      // Decrease-key on a random live node.
      auto& slot = live[std::uniform_int_distribution<size_t>(0, live.size() - 1)(rng)];
      Key_t new_key = slot.second - (key_dist(rng) & 0x3F) - 1;  // strictly smaller
      model.erase(model.find(slot.second));
      model.insert(new_key);
      slot.second = new_key;
      fibHeapOverrideKey(heap, slot.first, new_key);
    } else {
      // Delete a random live node.
      size_t idx = std::uniform_int_distribution<size_t>(0, live.size() - 1)(rng);
      FibNode* n = live[idx].first;
      model.erase(model.find(live[idx].second));
      fibHeapDel(heap, n);
      live.erase(live.begin() + static_cast<long>(idx));
    }

    check_min();
  }

  // Drain everything and confirm sorted order against the model.
  while (!model.empty()) {
    ASSERT_NE(fibHeapGetMin(heap), nullptr);
    EXPECT_EQ(fibHeapGetMin(heap)->key, *model.begin());
    model.erase(model.begin());
    fibHeapExtMin(heap);
  }
  EXPECT_EQ(fibHeapGetSize(heap), 0u);
  fibHeapDtor(heap);
}

INSTANTIATE_TEST_SUITE_P(Seeds, FibHeapModelTest,
                         ::testing::Values(1u, 2u, 7u, 42u, 1337u, 90210u));

}  // namespace
