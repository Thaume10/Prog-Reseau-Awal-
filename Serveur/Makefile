CC = gcc
CFLAGS = -Wall -O2
TARGET = server2
SRCS = server2.c jeu.c
OBJS = $(SRCS:.c=.o)
HEADERS = server2.h client2.h jeu.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) -lpthread -g -ljansson

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	rm -f $(TARGET) $(OBJS)
	@echo "Cleanup complete."
