#include <gtest/gtest.h>

#include <cstdint>
#include <vector>

extern "C" {
#include "Stack/Stack.h"
}

namespace {

// =====================================================================
// Constructor / destructor
// =====================================================================

TEST(Stack, CtorReturnsEmptyStackWithDefaultCapacity) {
  Stack* stk = stackCtor();
  ASSERT_NE(stk, nullptr);
  ASSERT_NE(stk->data, nullptr);
  EXPECT_EQ(stk->size, 0u);
  EXPECT_EQ(stk->capacity, static_cast<uint64_t>(STK_START_SIZE));
  EXPECT_EQ(stackDtor(stk), SE_ALL_OK);
}

TEST(Stack, DtorReturnsAllOkForValidStack) {
  Stack* stk = stackCtor();
  EXPECT_EQ(stackDtor(stk), SE_ALL_OK);
}

TEST(Stack, DtorReturnsPointerErrForNull) {
  EXPECT_EQ(stackDtor(nullptr), SE_POINTER_ERR);
}

TEST(Stack, DtorReturnsPointerErrWhenDataIsNull) {
  Stack* stk = stackCtor();
  free(stk->data);
  stk->data = nullptr;
  EXPECT_EQ(stackDtor(stk), SE_POINTER_ERR);
  // Real cleanup — Dtor refused to free, do it manually.
  free(stk);
}

// =====================================================================
// Push / pop / top / size — happy path
// =====================================================================

TEST(Stack, PushIncrementsSizeAndTopMatches) {
  Stack* stk = stackCtor();
  int a = 1, b = 2, c = 3;
  EXPECT_EQ(stackPush(stk, &a), SE_ALL_OK);
  EXPECT_EQ(stackSize(stk), 1u);
  EXPECT_EQ(stackTop(stk), &a);

  EXPECT_EQ(stackPush(stk, &b), SE_ALL_OK);
  EXPECT_EQ(stackSize(stk), 2u);
  EXPECT_EQ(stackTop(stk), &b);

  EXPECT_EQ(stackPush(stk, &c), SE_ALL_OK);
  EXPECT_EQ(stackTop(stk), &c);
  EXPECT_EQ(stackSize(stk), 3u);

  stackDtor(stk);
}

TEST(Stack, PopReducesSizeAndExposesPreviousTop) {
  Stack* stk = stackCtor();
  int a = 1, b = 2;
  stackPush(stk, &a);
  stackPush(stk, &b);

  EXPECT_EQ(stackTop(stk), &b);
  EXPECT_EQ(stackPop(stk), SE_ALL_OK);
  EXPECT_EQ(stackTop(stk), &a);
  EXPECT_EQ(stackSize(stk), 1u);

  EXPECT_EQ(stackPop(stk), SE_ALL_OK);
  EXPECT_EQ(stackSize(stk), 0u);

  stackDtor(stk);
}

// =====================================================================
// Capacity growth (push past STK_START_SIZE)
// =====================================================================

TEST(Stack, CapacityDoublesOnOverflow) {
  Stack* stk = stackCtor();
  std::vector<int> values(STK_START_SIZE + 1);

  for (std::size_t i = 0; i <= STK_START_SIZE; ++i) {
    values[i] = static_cast<int>(i);
    EXPECT_EQ(stackPush(stk, &values[i]), SE_ALL_OK);
  }
  EXPECT_EQ(stk->size, static_cast<uint64_t>(STK_START_SIZE + 1));
  EXPECT_EQ(stk->capacity, static_cast<uint64_t>(STK_START_SIZE) * 2);

  stackDtor(stk);
}

// =====================================================================
// Capacity shrink (pop until size < capacity/4)
// =====================================================================

TEST(Stack, CapacityHalvesWhenSparseEnough) {
  Stack* stk = stackCtor();
  // Push 2*START so capacity becomes 4*START; shrink kicks in once
  // size drops below START.
  std::vector<int> values(STK_START_SIZE * 2 + 1);
  for (std::size_t i = 0; i <= STK_START_SIZE * 2; ++i) {
    values[i] = static_cast<int>(i);
    stackPush(stk, &values[i]);
  }
  ASSERT_EQ(stk->capacity, static_cast<uint64_t>(STK_START_SIZE) * 4);

  // Pop down to STK_START_SIZE - 1; trigger shrink → capacity 2*START.
  while (stk->size >= static_cast<uint64_t>(STK_START_SIZE)) {
    stackPop(stk);
  }
  EXPECT_EQ(stk->capacity, static_cast<uint64_t>(STK_START_SIZE) * 2);

  // Pop down to STK_START_SIZE/2 - 1; second shrink → capacity = START.
  while (stk->size >= static_cast<uint64_t>(STK_START_SIZE) / 2) {
    stackPop(stk);
  }
  EXPECT_EQ(stk->capacity, static_cast<uint64_t>(STK_START_SIZE));

  // Further pops must NOT shrink below STK_START_SIZE.
  while (stk->size > 0) {
    stackPop(stk);
  }
  EXPECT_EQ(stk->capacity, static_cast<uint64_t>(STK_START_SIZE));

  stackDtor(stk);
}

// =====================================================================
// Empty / error paths
// =====================================================================

TEST(Stack, PopOnEmptyReturnsStackEmpty) {
  Stack* stk = stackCtor();
  EXPECT_EQ(stackPop(stk), SE_STACK_EMPTY_ERR);
  stackDtor(stk);
}

TEST(Stack, TopOnEmptyReturnsNull) {
  Stack* stk = stackCtor();
  EXPECT_EQ(stackTop(stk), nullptr);
  stackDtor(stk);
}

TEST(Stack, PushNullStackReturnsPointerErr) {
  int x = 0;
  EXPECT_EQ(stackPush(nullptr, &x), SE_POINTER_ERR);
}

TEST(Stack, PopNullStackReturnsPointerErr) {
  EXPECT_EQ(stackPop(nullptr), SE_POINTER_ERR);
}

TEST(Stack, TopNullStackReturnsNull) {
  EXPECT_EQ(stackTop(nullptr), nullptr);
}

TEST(Stack, SizeNullStackReturnsZero) {
  EXPECT_EQ(stackSize(nullptr), 0u);
}

TEST(Stack, PushWithNullDataPointerReturnsPointerErr) {
  Stack* stk = stackCtor();
  free(stk->data);
  stk->data = nullptr;
  int x = 0;
  EXPECT_EQ(stackPush(stk, &x), SE_POINTER_ERR);
  free(stk);
}

TEST(Stack, PopWithNullDataPointerReturnsPointerErr) {
  Stack* stk = stackCtor();
  int x = 0;
  stackPush(stk, &x);  // size becomes 1 first
  free(stk->data);
  stk->data = nullptr;
  EXPECT_EQ(stackPop(stk), SE_POINTER_ERR);
  free(stk);
}

TEST(Stack, TopWithNullDataPointerReturnsNull) {
  Stack* stk = stackCtor();
  int x = 0;
  stackPush(stk, &x);
  free(stk->data);
  stk->data = nullptr;
  EXPECT_EQ(stackTop(stk), nullptr);
  free(stk);
}

// =====================================================================
// LIFO over a representative sequence
// =====================================================================

TEST(Stack, PreservesLifoOrderUnderMixedOps) {
  Stack* stk = stackCtor();
  std::vector<int> values{10, 20, 30, 40, 50};

  for (int& v : values) {
    stackPush(stk, &v);
  }
  for (auto it = values.rbegin(); it != values.rend(); ++it) {
    EXPECT_EQ(stackTop(stk), &*it);
    stackPop(stk);
  }
  EXPECT_EQ(stackSize(stk), 0u);

  stackDtor(stk);
}

}  // namespace
