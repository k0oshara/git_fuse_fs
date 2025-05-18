CC         := gcc
PKG_CFLAGS := $(shell pkg-config --cflags fuse3 libgit2)
PKG_LIBS   := $(shell pkg-config --libs fuse3 libgit2)
CFLAGS     := -Wall -Wextra -pedantic -DFUSE_USE_VERSION=31 -Iinclude $(PKG_CFLAGS)
LDFLAGS    := $(PKG_LIBS)

SRCS   := main.c src/gitfs.c
OBJS   := $(SRCS:.c=.o)
TARGET := gitfs

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c include/gitfs.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
