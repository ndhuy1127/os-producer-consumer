CC ?= cc
CPPFLAGS += -Iinclude
CFLAGS ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CFLAGS += -pthread -MMD -MP
LDLIBS += -pthread
SOURCES := $(wildcard src/*.c)
OBJECTS := $(patsubst src/%.c,build/%.o,$(SOURCES))
TARGET := bin/producer-consumer
THREAD_TARGET := bin/thread-demo
THREAD_OBJECT := build/thread_lifecycle.o

.DEFAULT_GOAL := all
.PHONY: all clean test help thread-demo

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

thread-demo: $(THREAD_TARGET)

$(THREAD_TARGET): $(THREAD_OBJECT)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(THREAD_OBJECT): examples/thread_lifecycle.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

-include $(THREAD_OBJECT:.o=.d)

test:
	@printf '%s\n' 'Chua co bo kiem thu chuong trinh; xem docs/test-plan.md.' >&2
	@exit 2

clean:
	rm -rf -- build bin

help:
	@printf '%s\n' 'make: build src/*.c voi -pthread khi co ma nguon.' 'make thread-demo: build vi du tao/join luong vao bin/thread-demo.' 'make clean: don build/ va bin/.' 'make test: bao chua trien khai cho den khi co bo kiem thu.'
