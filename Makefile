CC := clang
CFLAGS := -std=c2y -Wall -Wextra -Wpedantic -Werror -Iinclude -O2
LDFLAGS :=

TARGET := vm8080

SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,build/%.o,$(SOURCES))

.PHONY: all clean run format

all: $(TARGET)

$(TARGET): $(OBJECTS) | build
	@$(CC) $(OBJECTS) $(LDFLAGS) -o $@

build/%.o: src/%.c | build
	@$(CC) $(CFLAGS) -c $< -o $@

build:
	@mkdir -p $@

format:
	@clang-format -i include/*.h src/*.c

clean:
	@$(RM) $(OBJECTS) $(TARGET)
	@$(RM) -r build
