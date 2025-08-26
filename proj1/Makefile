CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = testlib

SRCS = main.c list.c hash.c bitmap.c debug.c hex_dump.c


OBJS = $(SRCS:.c=.o)

DEPS = list.h hash.h bitmap.h debug.h hex_dump.h

STUDENT_ID = 20231552  


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
