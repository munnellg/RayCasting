OBJS = $(wildcard src/*.c)

CC = clang

LFLAGS = -lSDL2 -lm

CFLAGS = -Wall

TARGET = raycast

all : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) $(LFLAGS)
