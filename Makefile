# minimalist makefile
.SUFFIXES:
#
.SUFFIXES: .cpp .o .c .h
ifeq ($(DEBUG),1)
CFLAGS = -fPIC  -std=c99 -ggdb -march=native -Wall -Wextra -Wshadow -fsanitize=undefined  -fno-omit-frame-pointer -fsanitize=address
else
CFLAGS = -fPIC -std=c99 -O3  -march=native -Wall -Wextra -Wshadow

endif # debug
all: unit
HEADERS=include/fastmod.h
unit: ./tests/unit.c $(HEADERS)
	$(CC) $(CFLAGS) -o unit ./tests/unit.c -Iinclude
clean:
	rm -f  unit
