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
PC_TARGET := bin/pc-test
PC_OBJECTS := build/buffer.o build/pc.o build/test_pc.o
ERROR_TARGET := bin/pc-error-test
ERROR_OBJECTS := build/buffer.o build/pc.o build/test_pc_faults.o
ERROR_WRAPS := sem_init sem_wait sem_trywait sem_post sem_destroy pthread_create pthread_join malloc calloc free buffer_push buffer_pop
comma := ,

.DEFAULT_GOAL := all
.PHONY: all clean test help thread-demo buffer-test test-buffer pc-test pc-error-test test-pc test-errors

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

build/test_pc.o: tests/test_pc.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

build/test_pc_faults.o: tests/test_pc_faults.c
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

pc-test: $(PC_TARGET)

$(PC_TARGET): $(PC_OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

pc-error-test: $(ERROR_TARGET)

$(ERROR_TARGET): $(ERROR_OBJECTS)
	@mkdir -p $(@D)
	$(CC) $(LDFLAGS) $^ $(addprefix -Wl$(comma)--wrap=,$(ERROR_WRAPS)) $(LDLIBS) -o $@

test-pc: pc-test
	timeout 15s ./$(PC_TARGET) all

test-errors: pc-error-test
	python3 scripts/check_pc_errors.py

test: test-buffer test-pc test-errors

-include $(sort $(OBJECTS:.o=.d) $(BUFFER_OBJECTS:.o=.d) $(THREAD_OBJECT:.o=.d) $(PC_OBJECTS:.o=.d) $(ERROR_OBJECTS:.o=.d))

clean:
	rm -rf -- build bin

help:
	@printf '%s\n' 'make: build bin/producer-consumer (mac dinh N=8 P=C=1 K=20; CLI cho Huy).' 'make thread-demo: build bin/thread-demo.' 'make test-buffer: 12 ca bo dem tuan tu.' 'make test-pc: 8 ca loi dong bo 1P/1C, timeout 15s.' 'make test-errors: wrapper rieng, timeout tung ca.' 'make test: test-buffer + test-pc + test-errors.' 'make clean: don build/ va bin/.'
