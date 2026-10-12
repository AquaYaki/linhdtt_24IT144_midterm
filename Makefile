CC ?= gcc
CFLAGS = -Wall -Wextra -Wpedantic -std=c99 -Iinclude -D_POSIX_C_SOURCE=200809L -D_XOPEN_SOURCE=700

TARGET = ls
SYMLINK = myls

SRCS = src/main.c \
       src/parse_args.c \
       src/traverse.c \
       src/sort.c \
       src/display.c \
       src/utils.c

OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)
	ln -sf $(TARGET) $(SYMLINK) || cp $(TARGET) $(SYMLINK)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET) $(SYMLINK)

.PHONY: all clean
