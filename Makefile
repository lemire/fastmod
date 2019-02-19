# minimalist makefile
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h
ifeq ($(DEBUG),1)
CFLAGS = -fPIC  -std=c99 -ggdb -march=native -Wall -Wshadow -fsanitize=undefined  -fno-omit-frame-pointer -fsanitize=address
CXXFLAGS = -fPIC  -std=c++11 -ggdb -march=native -Wall  -Wshadow -fsanitize=undefined  -fno-omit-frame-pointer -fsanitize=address
else
CFLAGS = -fPIC -std=c99 -O3  -march=native -Wall  -Wshadow
CXXFLAGS = -fPIC  -std=c++11 -O3 -march=native -Wall  -Wshadow

endif # debug
all: unit
HEADERS=include/fastmod.h
unit: ./tests/unit.c $(HEADERS)
	$(CC) $(CFLAGS) -o unit ./tests/unit.c -Iinclude
%: ./tests/%.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -o $@ $< -Iinclude
benchmark: modnbenchmark moddivnbenchmark
clean:
	rm -f  unit modnbenchmark moddivnbenchmark
