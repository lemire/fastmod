function(add_cpp_test TEST_NAME)
  add_executable(${TEST_NAME} ${TEST_NAME}.cpp)
  target_link_libraries(${TEST_NAME} fastmod)
  add_test(${TEST_NAME} ${TEST_NAME})
endfunction(add_cpp_test)
add_library(cppincludetest1 STATIC cppincludetest1.cpp)
target_link_libraries(cppincludetest1 fastmod)
option(FASTMOD_EXHAUSTIVE_TESTS "Run exhaustive tests" OFF)
if(FASTMOD_EXHAUSTIVE_TESTS)
  add_cpp_test(cppincludetest2)
  target_link_libraries(cppincludetest2 cppincludetest1)  
endif(FASTMOD_EXHAUSTIVE_TESTS)
add_cpp_test(moddivnbenchmark)
add_cpp_test(modnbenchmark)