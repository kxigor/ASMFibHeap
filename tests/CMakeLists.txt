include(GtestTools)
if(NOT GTest_FOUND)
  message(WARNING "GTest not found — tests will not be built.")
  return()
endif()

add_subdirectory(unit)
add_subdirectory(differential)
add_subdirectory(stress)
