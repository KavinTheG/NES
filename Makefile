CC = gcc
OBJ = main.o cpu.o
TARGET = emulator

# Default rule to build the binary
all: $(TARGET)

# Link the object files to create the final binary
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Rule to compile main.c into main.o
main.o: main.c cpu.h
	$(CC) $(CFLAGS) -c main.c

# Rule to compile cpu.c into cpu.o
cpu.o: cpu.c cpu.h
	$(CC) $(CFLAGS) -c cpu.c

# Rule to clean up compiled files
clean:
	rm -f $(OBJ) $(TARGET)

# Rule to execute the binary (optional, but you can run `make run`)
run: $(TARGET)
	./$(TARGET)
