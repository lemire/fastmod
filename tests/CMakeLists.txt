add_executable(unit unit.c)
target_link_libraries(unit PRIVATE fastmod)
add_test(NAME unit COMMAND unit)

function(add_cpp_test TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
  target_link_libraries(${TEST_NAME} PRIVATE fastmod)
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_library(cppincludetest1 STATIC cppincludetest1.cpp)
target_link_libraries(cppincludetest1 PRIVATE fastmod)

option(FASTMOD_EXHAUSTIVE_TESTS "Run exhaustive tests" OFF)
if(FASTMOD_EXHAUSTIVE_TESTS)
  add_cpp_test(cppincludetest2)
  target_link_libraries(cppincludetest2 PRIVATE cppincludetest1)
endif()

add_cpp_test(moddivnbenchmark)
add_cpp_test(modnbenchmark)
