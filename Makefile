CC = cc

OPENSSL_DIR = /usr/local/opt/openssl@1.1/
OPENSSL_SUPPORT = -I$(OPENSSL_DIR)/include -L$(OPENSSL_DIR)/lib
CFLAGS = -std=c11 -fsanitize=address -O0 -g -Iinclude -I/usr/include/postgresql $(OPENSSL_SUPPORT) -Wall -pedantic -Wno-unused-command-line-argument -pthread -lpq -lssl -lcrypto
SRC_FILES = sha256 smtp util auth_client cli
OBJ_FILES = $(addprefix obj/,$(SRC_FILES:=.o))

MAIN = main schema
MAIN_BINS = $(addprefix bin/,$(MAIN))
TEST_BINS = $(addprefix bin/test_,$(SRC_FILES))

all: $(MAIN_BINS) $(TEST_BINS)

obj:
	mkdir obj

bin:
	mkdir bin

bin/%: main/%.c $(OBJ_FILES) | bin
	$(CC) $(CFLAGS) $^ -o $@

obj/%.o: src/%.c | obj
	$(CC) -c $(CFLAGS) $^ -o $@
obj/%.o: main/%.c | obj
	$(CC) -c $(CFLAGS) $^ -o $@
obj/%.o: tests/%.c | obj
	$(CC) -c $(CFLAGS) $^ -o $@
	
bin/test_%: tests/test_%.c $(OBJ_FILES) | bin
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -rf bin
	rm -rf obj

test: $(TEST_BINS)
	@for f in $(TEST_BINS); do echo $$f; ASAN_OPTIONS=detect_leaks=1 $$f; echo; done

memcheck:
	ASAN_OPTIONS=detect_leaks=1 ./bin/main

.PHONY: all clean
