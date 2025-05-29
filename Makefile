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

.PHONY: all clean lint

all: $(TARGET)

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
	rm -f $(OBJS) $(TARGET)
