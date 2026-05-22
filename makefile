CC := clang
CCOPT := -Wall -Wextra -Werror -g -fPIC

HIGHLIGHERS_C   := $(wildcard src/highlighters/*.c)
HIGHLIGHTERS_SO := $(patsubst src/highlighters/%.c,build/highlighters/%.so,$(HIGHLIGHERS_C))

all: build/madlad build/highlighters build/dsh

build/:
	mkdir -p build
	mkdir -p build/highlighters
	mkdir -p build/dsh

build/main.o: src/main.c src/ed.h src/constants.h | build/
	$(CC) $(CCOPT) -c -o $@ $<

build/buffer.o: src/buffer.c src/buffer.h src/constants.h | build/
	$(CC) $(CCOPT) -c -o $@ $<

build/ed.o: src/ed.c src/ed.h src/buffer.h src/cmds.h src/constants.h | build/
	$(CC) $(CCOPT) -c -o $@ $<

build/cmds.o: src/cmds.c src/cmds.h src/ed.h src/constants.h lib/csrpc | build/
	$(CC) $(CCOPT) -c -o $@ $< -I./lib/csrpc/include

build/madlad: build/main.o build/buffer.o build/ed.o build/cmds.o
	$(CC) -o $@ $^ -L./lib/csrpc/build/ -lcsrpc

build/highlighters/%.so: src/highlighters/%.c | build/
	$(CC) $(CCOPT) -shared -o $@ $<

build/highlighters: $(HIGHLIGHTERS_SO)

.PHONY: lib/csrpc
lib/csrpc:
	$(MAKE) -C $@

.PHONY: build/dsh
build/dsh:
	rm -rf build/dsh/
	mkdir -p build/dsh
	cp lib/csrpc/build/send-cmd build/dsh/sc
	cp -r src/dsh/* build/dsh/
	chmod +x build/dsh/*

.PHONY: run
run: all
	build/madlad

.PHONY: dbg
dbg: build/madlad
	gdb $^

.PHONY: clean
clean: | build/
	rm -rf build/
