CC = gcc
CFLAGS = -Wall -Wextra

TARGET = cshell
SRCS = cshell.c cmd.c clog.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $^ -o $@

clean:
	rm -f $(TARGET)
