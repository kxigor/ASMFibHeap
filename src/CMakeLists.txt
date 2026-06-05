add_subdirectory(Stack)
add_subdirectory(FibHeap)
add_subdirectory(Graph)

if(ASMFIBHEAP_BUILD_ASM)
  add_subdirectory(ASM_FibHeap)
endif()

# NOTE: the old src/Testing harness (the scanf-driven MAKE_TESTING macro and
# the boost-based AnswersGenerator) has been retired — the interactive demo
# now lives in apps/ and differential testing is covered by tests/differential.
