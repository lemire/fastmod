option(FASTMOD_EXHAUSTIVE_TESTS "Run exhaustive tests" OFF)
option(FASTMOD_QUICK_TESTS "Run a reduced (quick) version of the tests suitable for CI" OFF)

add_executable(unit unit.c)
target_link_libraries(unit PRIVATE fastmod)
if(FASTMOD_QUICK_TESTS)
  target_compile_definitions(unit PRIVATE FASTMOD_QUICK_TESTS)
endif()
add_test(NAME unit COMMAND unit)

function(add_cpp_test TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
  target_link_libraries(${TEST_NAME} PRIVATE fastmod)
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endfunction()

add_library(cppincludetest1 STATIC cppincludetest1.cpp)
target_link_libraries(cppincludetest1 PRIVATE fastmod)

if(FASTMOD_EXHAUSTIVE_TESTS AND NOT FASTMOD_QUICK_TESTS)
  add_cpp_test(cppincludetest2)
  target_link_libraries(cppincludetest2 PRIVATE cppincludetest1)
endif()

add_cpp_test(moddivnbenchmark)
add_cpp_test(modnbenchmark)
