CC := clang
CCOPT := -Wall -Wextra -Werror -g

all: build/madlad

build/:
	mkdir -p build/

build/main.o: src/main.c src/buffer.h src/constants.h build/
	$(CC) $(CCOPT) -c -o $@ $<

build/buffer.o: src/buffer.c src/buffer.h src/constants.h build/
	$(CC) $(CCOPT) -c -o $@ $<

build/madlad: build/main.o build/buffer.o
	$(CC) -o $@ $^

.PHONY: run
run: build/madlad
	$^

.PHONY: dbg
dbg: build/madlad
	gdb $^
