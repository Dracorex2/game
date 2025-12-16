# Compiler
CC = gcc
CFLAGS = -std=c11 -O2 -Iinclude -Idependencies -Idependencies/onion/Iinclude

# Libraries
LIBS = -lglfw -ldl -lGL -lm -pthread

# Sources
SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c, obj/%.o, $(SRC))
OBJ += obj/glad.o obj/lodepng.o


# Exécutable
TARGET = cube

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(LIBS) $(OBJ) -o $@

obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

obj/glad.o: dependencies/glad/glad.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

obj/lodepng.o: dependencies/lodepng/lodepng.c
	@mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj

fclean: clean
	rm -f $(TARGET)

re: fclean all

.PHONY: re fclean clean all
