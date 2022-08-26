##
# Ika
#
# @file
# @version 0.1
#
TARGET = build/ika
LIBS = -lm
CC = clang
LD = ldd
CFLAGS = -g -Wall -pedantic -Werror -std=c11 -Qunused-arguments -Wgnu-zero-variadic-macro-arguments
BUILD_DIR = build

.PHONY: clean all default

default: $(TARGET)
all: show_env build_dir default

build_dir:
	mkdir -p ${BUILD_DIR}

show_env:
	echo ${OBJECTS}

OBJECTS = $(patsubst src/%.c, src/%.o, $(wildcard src/*.c))
HEADERS = $(wildcard src/*.h)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -Wall $(LIBS) -o $@

clean:
	-rm -f ${BUILD_DIR}
	-rm -f $(TARGET)

# end
