CC = gcc
OBJ = ./build/main.o ./build/cpu.o ./build/ppu.o ./build/queue.o 
CFLAGS = -Wall -Iinclude -lSDL2
TARGET = ./bin/emulator
VPATH = src

# Default rule to build the binary
all: $(TARGET)

# Link the object files to create the final binary
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Rule to compile main.c into main.o
./build/main.o: main.c | ./build
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile cpu.c into cpu.o
./build/cpu.o: cpu.c | ./build
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to compile ppu.c into ppu.o
./build/ppu.o: ppu.c | ./build
	$(CC) $(CFLAGS) -c $< -o $@

./build/queue.o: queue.c | ./build
	$(CC) $(CFLAGS) -c $< -o $@

# Rule to clean up compiled files
clean:
	rm -f $(OBJ) $(TARGET)

# Rule to execute the binary (optional, but you can run `make run`)
run: $(TARGET)
	./$(TARGET)
