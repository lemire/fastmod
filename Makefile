# minimalist makefile
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h
ifeq ($(DEBUG),1)
CFLAGS = -fPIC  -std=c99 -ggdb  -Wall -Wshadow -fsanitize=undefined  -fno-omit-frame-pointer -fsanitize=address
CXXFLAGS = -fPIC  -std=c++11 -ggdb  -Wall  -Wshadow -fsanitize=undefined  -fno-omit-frame-pointer -fsanitize=address
else
CFLAGS = -fPIC -std=c99 -O3   -Wall  -Wshadow
CXXFLAGS = -fPIC  -O3  -Wall  -Wshadow

endif # debug
all: unit cppincludetest2 cppincludetest203
HEADERS=include/fastmod.h

unit: ./tests/unit.c $(HEADERS)
	$(CC) $(CFLAGS) -o unit ./tests/unit.c -Iinclude

cppincludetest1.o: tests/cppincludetest1.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -std=c++11 -c ./tests/cppincludetest1.cpp -Iinclude

cppincludetest2: cppincludetest1.o tests/cppincludetest2.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -std=c++11 -o cppincludetest2 ./tests/cppincludetest2.cpp cppincludetest1.o -Iinclude

cppincludetest103.o: tests/cppincludetest1.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -std=c++03 -o cppincludetest103.o -c ./tests/cppincludetest1.cpp -Iinclude

cppincludetest203: cppincludetest103.o tests/cppincludetest2.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -std=c++03 -o cppincludetest2 ./tests/cppincludetest2.cpp cppincludetest103.o -Iinclude

%: ./tests/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $< -Iinclude

benchmark: modnbenchmark moddivnbenchmark


clean:
	rm -f  unit modnbenchmark moddivnbenchmark cppincludetest2 cppincludetest203 cppincludetest1.o cppincludetest103.o
