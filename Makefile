SDL2_CONFIG := sdl2-config
CFLAGS  := $(shell $(SDL2_CONFIG) --cflags)
LDFLAGS := $(shell $(SDL2_CONFIG) --libs)

CFLAGS  += -Iinclude -Wall -Wextra -O2 -g \
           -D_POSIX_C_SOURCE=200809L -D_GNU_SOURCE -pthread
LDFLAGS += -lm -pthread

CC := gcc

SRC := src/main.c src/rtdb.c src/buffer.c src/desc_queue.c \
       src/audio_io.c src/dispatcher.c src/speed.c src/display.c \
	   src/lpf.c src/fft.c src/bearing.c
OBJ := $(SRC:.c=.o)

BIN    := bin
TARGET := $(BIN)/audio_app

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p $(BIN)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

run: $(TARGET)
	@clear
	# passa o índice do dispositivo (ex.: 0)
	$(TARGET) 0

