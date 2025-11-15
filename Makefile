# Lseco - Secure Memory Library
# Compiler and flags with security hardening

CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC \
         -fstack-protector-all \
         -D_FORTIFY_SOURCE=2 \
         -fvisibility=hidden

# Platform-specific flags
ifeq ($(OS),Windows_NT)
    # Windows (MinGW)
    CFLAGS += -D_WIN32_WINNT=0x0600 # For VirtualAlloc, VirtualLock
    LDFLAGS = -lws2_32
    SHARED_EXT = dll
    SHARED_FLAGS = -shared
    TEST_RUN_CMD = $(TEST_BINARY).exe
else
    # Linux/macOS
    UNAME_S := $(shell uname -s)
    ifeq ($(UNAME_S),Linux)
        CFLAGS += -fcf-protection=full
        LDFLAGS = -Wl,-z,relro,-z,now -Wl,-z,noexecstack
        SHARED_EXT = so
        SHARED_FLAGS = -shared
        TEST_RUN_CMD = LD_LIBRARY_PATH=. ./$(TEST_BINARY)
    endif
    ifeq ($(UNAME_S),Darwin)
        LDFLAGS = -Wl,-bind_at_load
        SHARED_EXT = dylib
        SHARED_FLAGS = -dynamiclib
        TEST_RUN_CMD = DYLD_LIBRARY_PATH=. ./$(TEST_BINARY)
    endif
endif

# Clang-specific hardening
ifeq ($(CC),clang)
    CFLAGS += -fsanitize=safe-stack
endif

# Output library name
LIB_NAME = liblseco
STATIC_LIB = $(LIB_NAME).a
SHARED_LIB = $(LIB_NAME).$(SHARED_EXT)

# Source and object files
SOURCES = secure_memory.c lseco_ffi.c
OBJECTS = $(SOURCES:.c=.o)
TEST_SOURCES = test_lseco.c
TEST_BINARY = test_lseco

# Default target
all: $(STATIC_LIB) $(SHARED_LIB)

# Compile object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Build static library
$(STATIC_LIB): $(OBJECTS)
	ar rcs $@ $^
	@echo "Built static library: $(STATIC_LIB)"

# Build shared library
$(SHARED_LIB): $(OBJECTS)
	$(CC) $(SHARED_FLAGS) $(LDFLAGS) -o $@ $^
	@echo "Built shared library: $(SHARED_LIB)"

# Build test program
test: $(SHARED_LIB) $(TEST_SOURCES)
	$(CC) $(CFLAGS) -L. -o $(TEST_BINARY) $(TEST_SOURCES) -l:$(SHARED_LIB)
	@echo "Built test program: $(TEST_BINARY)"

# Run test
run-test: test
	$(TEST_RUN_CMD)

# Clean build artifacts
clean:
	-rm -f $(OBJECTS) $(STATIC_LIB) $(SHARED_LIB) $(TEST_BINARY) $(TEST_BINARY).exe
	@echo "Cleaned build artifacts"

# Install/Uninstall are not supported on Windows via this Makefile
ifeq ($(OS),Windows_NT)
install uninstall:
	@echo "make install/uninstall is not supported on Windows."
else
# Install library (requires sudo on most systems)
install: $(STATIC_LIB) $(SHARED_LIB)
	install -d /usr/local/lib
	install -m 644 $(STATIC_LIB) /usr/local/lib/
	install -m 755 $(SHARED_LIB) /usr/local/lib/
	install -d /usr/local/include
	install -m 644 lseco_ffi.h /usr/local/include/
	@echo "Installed library to /usr/local"

# Uninstall library
uninstall:
	rm -f /usr/local/lib/$(STATIC_LIB)
	rm -f /usr/local/lib/$(SHARED_LIB)
	rm -f /usr/local/include/lseco_ffi.h
	@echo "Uninstalled library"
endif

# Help
help:
	@echo "Lseco Secure Memory Library - Build Targets:"
	@echo "  make all         - Build static and shared libraries (default)"
	@echo "  make test        - Build test program"
	@echo "  make run-test    - Build and run test program"
	@echo "  make clean       - Remove build artifacts"
	@echo "  make install     - Install library system-wide (needs sudo)"
	@echo "  make uninstall   - Uninstall library"
	@echo ""
	@echo "Security Hardening Features:"
	@echo "  - Stack Protector (all functions)"
	@echo "  - ASLR/PIE support"
	@echo "  - Full RELRO (Linux)"
	@echo "  - Control Flow Protection (Linux/Intel)"
	@echo "  - Non-executable stack"

.PHONY: all test run-test clean install uninstall help
