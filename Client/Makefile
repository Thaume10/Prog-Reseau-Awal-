CC = gcc
CFLAGS = -Wall -O2
TARGET = client2
SRCS = client2.c
OBJS = $(SRCS:.c=.o)
HEADERS = client2.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) $(OBJS)
