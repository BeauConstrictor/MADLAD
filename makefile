CC := clang
CCOPT := -Wall -Wextra -Werror -g -fPIC

HIGHLIGHERS_C   := $(wildcard src/highlighters/*.c)
HIGHLIGHTERS_SO := $(patsubst src/highlighters/%.c,build/highlighters/%.so,$(HIGHLIGHERS_C))

all: build/madlad build/highlighters

build/:
	mkdir -p build
	mkdir -p build/highlighters

build/main.o: src/main.c src/buffer.h src/constants.h | build/
	$(CC) $(CCOPT) -c -o $@ $<

build/buffer.o: src/buffer.c src/buffer.h src/constants.h | build/
	$(CC) $(CCOPT) -c -o $@ $<

build/madlad: build/main.o build/buffer.o
	$(CC) -o $@ $^

build/highlighters/%.so: src/highlighters/%.c | build/
	$(CC) $(CCOPT) -shared -o $@ $<

.PHONY: build/highlighters
build/highlighters: $(HIGHLIGHTERS_SO)

.PHONY: run
run: build/madlad
	$^

.PHONY: dbg
dbg: build/madlad
	gdb $^
