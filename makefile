CC := clang
CCOPT := -g -O0 -Wall -Wextra -Werror -g -fPIC -fsanitize=address,undefined -fno-omit-frame-pointer -fno-optimize-sibling-calls
LDOPT := -fsanitize=address,undefined

HIGHLIGHERS_C := $(wildcard src/highlighters/*.c)
HIGHLIGHTERS_SO := $(patsubst src/highlighters/%.c,build/highlighters/%.so,$(HIGHLIGHERS_C))

all: build/madlad build/highlighters build/dsh

.PHONY: build/
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
	$(CC) $(LDOPT) -o $@ $^ -L./lib/csrpc/build/ -lcsrpc

build/highlighters/%.so: src/highlighters/%.c | build/
	$(CC) $(CCOPT) -shared -o $@ $<

.PHONY: build/highlighters
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
run: export MADLAD_INSTALL=./build
run: ASAN_OPTIONS=symbolize=1:detect_leaks=1:verbosity=1
run: all
	./build/madlad

.PHONY: dbg
dbg: build/madlad
	gdb $^

.PHONY: clean
clean: | build/
	rm -rf build/

.PHONY: install
install: all
	mkdir -p $(HOME)/.local/share
	mkdir -p $(HOME)/.local/bin
	cp -r build/ $(HOME)/.local/share/madlad/
	cp build/madlad $(HOME)/.local/bin/madlad
