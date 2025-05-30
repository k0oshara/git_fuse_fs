CC         := gcc
PKG_CFLAGS := $(shell pkg-config --cflags fuse3 libgit2)
PKG_LIBS   := $(shell pkg-config --libs fuse3 libgit2)

CFLAGS     := -Wall -Wextra -pedantic -DFUSE_USE_VERSION=31 -Iinclude $(PKG_CFLAGS)
LDFLAGS    :=
LDLIBS     := $(PKG_LIBS)

SRCS       := main.c src/gitfs.c
OBJS       := $(SRCS:.c=.o)
TARGET     := gitfs

INC_DIR    := include
INC_HDRS   := $(INC_DIR)/gitfs.h $(INC_DIR)/uthash.h

.PHONY: all clean lint test test-full test-basic test-auto test-performance

all: $(TARGET)

test: $(TARGET) test/test_gitfs
	@echo "Running GitFS tests..."
	cd test && ./test_gitfs

test-full: $(TARGET) test/test_gitfs
	@echo "Running full test suite..."
	./scripts/run_tests.sh

test-basic: $(TARGET) test/test_gitfs
	@echo "Running basic tests..."
	./scripts/run_tests.sh --basic

test-auto: $(TARGET) test/test_gitfs
	@echo "Running automated tests..."
	./scripts/run_tests.sh --auto

test-performance: $(TARGET) test/test_gitfs
	@echo "Running performance tests..."
	./scripts/run_tests.sh --performance

test/test_gitfs: test/test_gitfs.c
	$(CC) $(CFLAGS) -o $@ $< $(LDLIBS)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(INC_DIR):
	mkdir -p $(INC_DIR)

$(INC_DIR)/uthash.h: | $(INC_DIR)
	@echo "Downloading uthash.h into $(INC_DIR)/"
	wget -qO $@ https://raw.githubusercontent.com/troydhanson/uthash/master/src/uthash.h

%.o: %.c $(INC_HDRS)
	$(CC) $(CFLAGS) -c $< -o $@

lint:
	find . -name '*.[ch]' -exec clang-format -i --style=file {} +

clean:
	rm -f $(OBJS) $(TARGET) test/test_gitfs
	rm -rf test-repo test-mount
