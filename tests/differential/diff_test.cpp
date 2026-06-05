// Differential test: drive identical, randomized operation streams through
// the pure-C FibHeap and the NASM ASM_FibHeap, and cross-check both against
// a std::multiset reference model after every operation. Any divergence in
// the reported minimum or size fails the test — this is the contract that
// guarantees the assembly port matches the C reference.
#include <gtest/gtest.h>

#include <algorithm>
#include <cstdint>
#include <random>
#include <set>
#include <vector>

extern "C" {
#include "ASM_FibHeap/ASM_FibHeap.h"
#include "FibHeap/FibHeap.h"
}

namespace {

// One logical element, tracked as the matching node in each heap.
struct Live {
  FibNode* c;
  ASM_FibNode* a;
  Key_t key;
};

class DiffTest : public ::testing::TestWithParam<unsigned> {};

TEST_P(DiffTest, CAndAsmAgreeWithModel) {
  std::mt19937 rng(GetParam());

  FibHeap* ch = fibHeapCtor();
  ASM_FibHeap* ah = ASM_fibHeapCtor();
  std::multiset<Key_t> model;
  std::vector<Live> live;
  std::set<Key_t> used;  // keys are kept globally unique → min is unambiguous

  auto fresh_key = [&](Key_t lo, Key_t hi) -> Key_t {
    std::uniform_int_distribution<Key_t> d(lo, hi);
    for (int tries = 0; tries < 64; ++tries) {
      Key_t k = d(rng);
      if (used.insert(k).second) return k;
    }
    return INT64_MIN;  // signal "could not find a unique key"
  };

  auto compare = [&]() {
    ASSERT_EQ(fibHeapGetSize(ch), model.size());
    ASSERT_EQ(ASM_fibHeapGetSize(ah), model.size());
    if (model.empty()) {
      ASSERT_EQ(fibHeapGetMin(ch), nullptr);
      ASSERT_EQ(ASM_fibHeapGetMin(ah), nullptr);
    } else {
      ASSERT_NE(fibHeapGetMin(ch), nullptr);
      ASSERT_NE(ASM_fibHeapGetMin(ah), nullptr);
      EXPECT_EQ(fibHeapGetMin(ch)->key, *model.begin());
      EXPECT_EQ(ASM_fibHeapGetMin(ah)->key, *model.begin());
    }
  };

  std::uniform_int_distribution<int> op_dist(0, 99);

  for (int step = 0; step < 6000; ++step) {
    int op = op_dist(rng);

    if (op < 45 || live.empty()) {
      // ---- insert ----
      Key_t k = fresh_key(-2'000'000, 2'000'000);
      if (k == INT64_MIN) continue;
      FibNode* cn = fibHeapIns(ch, k);
      ASM_FibNode* an = ASM_fibHeapIns(ah, k);
      live.push_back({cn, an, k});
      model.insert(k);

    } else if (op < 68) {
      // ---- extract-min ---- (unique keys → both heaps drop the same element)
      Key_t mn = *model.begin();
      model.erase(model.begin());
      used.erase(mn);
      fibHeapExtMin(ch);
      ASM_fibHeapExtMin(ah);
      auto it = std::find_if(live.begin(), live.end(),
                             [&](const Live& l) { return l.key == mn; });
      ASSERT_NE(it, live.end());
      live.erase(it);

    } else if (op < 85) {
      // ---- decrease-key ----
      size_t idx = std::uniform_int_distribution<size_t>(0, live.size() - 1)(rng);
      Live& slot = live[idx];
      Key_t lo = slot.key - 64;
      Key_t hi = slot.key - 1;
      if (hi < lo) continue;
      std::uniform_int_distribution<Key_t> d(lo, hi);
      Key_t nk = INT64_MIN;
      for (int t = 0; t < 64; ++t) {
        Key_t cand = d(rng);
        if (used.find(cand) == used.end()) { nk = cand; break; }
      }
      if (nk == INT64_MIN) continue;
      used.erase(slot.key);
      used.insert(nk);
      model.erase(model.find(slot.key));
      model.insert(nk);
      fibHeapOverrideKey(ch, slot.c, nk);
      ASM_fibHeapOverrideKey(ah, slot.a, nk);
      slot.key = nk;

    } else if (op < 95) {
      // ---- delete arbitrary node ----
      size_t idx = std::uniform_int_distribution<size_t>(0, live.size() - 1)(rng);
      Live slot = live[idx];
      model.erase(model.find(slot.key));
      used.erase(slot.key);
      fibHeapDel(ch, slot.c);
      ASM_fibHeapDel(ah, slot.a);
      live.erase(live.begin() + static_cast<long>(idx));

    } else {
      // ---- merge a freshly built heap into the main one ----
      FibHeap* c2 = fibHeapCtor();
      ASM_FibHeap* a2 = ASM_fibHeapCtor();
      int extra = std::uniform_int_distribution<int>(0, 8)(rng);
      for (int j = 0; j < extra; ++j) {
        Key_t k = fresh_key(-2'000'000, 2'000'000);
        if (k == INT64_MIN) continue;
        FibNode* cn = fibHeapIns(c2, k);
        ASM_FibNode* an = ASM_fibHeapIns(a2, k);
        live.push_back({cn, an, k});
        model.insert(k);
      }
      fibHeapMerge(ch, c2);
      ASM_fibHeapMerge(ah, a2);
    }

    compare();
  }

  // Drain both heaps; the extraction order must match the sorted model.
  while (!model.empty()) {
    Key_t mn = *model.begin();
    ASSERT_NE(fibHeapGetMin(ch), nullptr);
    ASSERT_NE(ASM_fibHeapGetMin(ah), nullptr);
    EXPECT_EQ(fibHeapGetMin(ch)->key, mn);
    EXPECT_EQ(ASM_fibHeapGetMin(ah)->key, mn);
    model.erase(model.begin());
    fibHeapExtMin(ch);
    ASM_fibHeapExtMin(ah);
  }
  EXPECT_EQ(fibHeapGetSize(ch), 0u);
  EXPECT_EQ(ASM_fibHeapGetSize(ah), 0u);

  fibHeapDtor(ch);
  ASM_fibHeapDtor(ah);
}

INSTANTIATE_TEST_SUITE_P(Seeds, DiffTest,
                         ::testing::Values(1u, 13u, 99u, 2024u, 777u));

}  // namespace
