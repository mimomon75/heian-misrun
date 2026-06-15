CC := gcc
CFLAGS := -O2 -std=gnu11 -Wall -Wextra
SRC := src/main.c

# Detect OS and set target name, libraries and run command
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
	TARGET := yaburi
	LIBS := -lraylib -lGL -lm -lpthread -ldl -lrt -lX11
	RUN := ./$(TARGET)
else ifeq ($(OS),Windows_NT)
	TARGET := yaburi.exe
	LIBS := -lraylib -lopengl32 -lgdi32 -lwinmm -lm
	RUN := $(TARGET)
else
	# Fallback
	TARGET := yaburi
	LIBS := -lraylib -lGL -lm
	RUN := ./$(TARGET)
endif

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET) $(LIBS)

run: $(TARGET)
	$(RUN)

makerun: run

clean:
	rm -f $(TARGET)

.PHONY: all run makerun clean

