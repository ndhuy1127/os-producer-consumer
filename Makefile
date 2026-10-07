CC ?= cc
CPPFLAGS += -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CFLAGS += -pthread -MMD -MP
LDLIBS += -pthread
SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,build/%.o,$(SOURCES))
TARGET := bin/producer-consumer

.DEFAULT_GOAL := all
.PHONY: all clean test help

ifeq ($(strip $(SOURCES)),)
all:
	@printf '%s\n' 'Chua co ma nguon C; chuong trinh chua duoc trien khai.' >&2
	@exit 2
else
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(OBJECTS:.o=.d)
endif

test:
	@printf '%s\n' 'Chua co bo kiem thu chuong trinh; xem docs/test-plan.md.' >&2
	@exit 2

clean:
	rm -rf -- build bin

help:
	@printf '%s\n' 'make: build src/*.c voi -pthread khi co ma nguon.' 'make clean: don build/ va bin/.' 'make test: bao chua trien khai cho den khi co bo kiem thu.'
