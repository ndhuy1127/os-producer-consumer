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
BUFFER_TARGET := bin/buffer-test
BUFFER_OBJECTS := build/buffer.o build/test_buffer.o

.DEFAULT_GOAL := all
.PHONY: all clean test help thread-demo buffer-test test-buffer

ifeq ($(wildcard src/main.c),)
all:
	@printf '%s\n' 'Chua co src/main.c; chuong trinh Producer-Consumer chua duoc trien khai.' >&2
	@exit 2
else
all: $(TARGET)

$(TARGET): $(OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@
endif

build/%.o: src/%.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(THREAD_OBJECT): examples/thread_lifecycle.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

thread-demo: $(THREAD_TARGET)

$(THREAD_TARGET): $(THREAD_OBJECT)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

build/test_buffer.o: tests/test_buffer.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -DBUFFER_TEST_WRAP_ALLOC -c $< -o $@

buffer-test: $(BUFFER_TARGET)

$(BUFFER_TARGET): $(BUFFER_OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ -Wl,--wrap=malloc,--wrap=free $(LDLIBS) -o $@

test-buffer: buffer-test
	@printf '%s\n' 'Pham vi: bo dem FIFO TUAN TU; chua kiem thu Producer-Consumer/semaphore/CLI.'
	./$(BUFFER_TARGET)

test: test-buffer

-include $(sort $(OBJECTS:.o=.d) $(BUFFER_OBJECTS:.o=.d) $(THREAD_OBJECT:.o=.d))

clean:
	rm -rf -- build bin

help:
	@printf '%s\n' 'make: build Producer-Consumer khi co src/main.c; hien chua trien khai.' 'make thread-demo: build vi du tao/join luong vao bin/thread-demo.' 'make buffer-test: build bin/buffer-test.' 'make test-buffer / make test: kiem thu bo dem tuan tu, khong kiem thu dong thoi.' 'make clean: don build/ va bin/.'
