# Lseco - Project Structure

```
lseco/
├── LICENSE                    # License file
├── README.md                  # Main documentation
├── .gitignore                # Git ignore rules
│
├── secure_memory.h            # Core secure memory interface
├── secure_memory.c            # Core implementation (POSIX/Windows)
│
├── lseco_ffi.h               # FFI public API
├── lseco_ffi.c               # FFI implementation
│
├── test_lseco.c              # Test suite
├── Makefile                   # Build system
│
├── liblseco.a                # Static library (generated)
├── liblseco.so/.dylib        # Shared library (generated)
├── test_lseco                # Test binary (generated)
│
└── examples/
    └── go/
        ├── README.md          # Go example documentation
        ├── Makefile           # Go example build
        ├── go.mod             # Go module file
        └── main.go            # Go FFI example
```

## Architecture

### Layer 1: Core Security Layer (`secure_memory.c/h`)
- Page-aligned memory allocation
- Memory protection (`mprotect` / `VirtualProtect`)
- RAM locking (`mlock` / `VirtualLock`)
- Secure zeroing (compiler-resistant)
- Core dump prevention (`MADV_DONTDUMP`)

### Layer 2: FFI Interface (`lseco_ffi.c/h`)
- Input validation (NULL checks, bounds)
- Error code translation
- Handle management
- Platform abstraction
- No crashes (never `exit`/`abort`)

### Layer 3: Language Bindings (`examples/`)
- Go CGo wrapper
- PHP FFI example (in README)
- Node.js N-API (future)
- Python ctypes (future)

## Security Implementation

### Memory Lifecycle

```
1. Create:  posix_memalign() → mlock() → mprotect(NOACCESS)
2. Write:   mprotect(READWRITE) → memcpy() → mprotect(NOACCESS)
3. Read:    mprotect(READ) → memcpy() → mprotect(NOACCESS)
4. Destroy: mprotect(READWRITE) → secure_zero() → munlock() → free()
```

### Compilation Pipeline

```
Source Code (.c)
    ↓
[Compiler Flags]
    ├─ -fstack-protector-all   (stack canaries)
    ├─ -fPIC                    (ASLR support)
    ├─ -fcf-protection          (CFI)
    └─ -D_FORTIFY_SOURCE=2      (buffer overflow detection)
    ↓
Object Files (.o)
    ↓
[Linker Flags]
    ├─ -Wl,-z,relro,-z,now      (GOT hardening)
    └─ -Wl,-z,noexecstack       (NX bit)
    ↓
Shared Library (.so/.dylib)
```

## API Design Principles

### 1. Memory Management
- **C allocates → C frees**
- Caller must call `lseco_destroy()`
- No automatic cleanup (GC cannot see C memory)

### 2. Error Handling
- Return error codes, never crash
- No `exit()`, `abort()`, or exceptions
- Safe for servers (no DoS risk)

### 3. Input Validation
- NULL pointer checks on every input
- Size bounds validation
- Length parameters for all buffers

### 4. Thread Safety
- Create/destroy are thread-safe
- Store/retrieve require external locks
- Documented clearly in API

## Build Targets

| Target | Description |
|--------|-------------|
| `make all` | Build static + shared library |
| `make test` | Build test program |
| `make run-test` | Build and run tests |
| `make clean` | Remove build artifacts |
| `make install` | Install system-wide (sudo) |
| `make uninstall` | Remove system install |

## Testing Strategy

### Unit Tests (`test_lseco.c`)
- ✅ Version and error strings
- ✅ Create/destroy lifecycle
- ✅ Store/retrieve operations
- ✅ NULL pointer validation
- ✅ Size limit enforcement
- ✅ Multiple operations
- ✅ Binary data integrity

### Integration Tests
- ✅ Go FFI (`examples/go/main.go`)
- 🔲 PHP FFI (manual test via README)
- 🔲 Memory leak check (Valgrind)
- 🔲 Thread safety (stress test)

### Security Tests (Manual)
```bash
# Check hardening flags
readelf -a liblseco.so | grep -E 'RELRO|BIND_NOW'  # Linux
otool -l liblseco.dylib | grep -E 'bind_at_load'  # macOS

# Check stack protection
nm liblseco.so | grep stack_chk                    # Linux/macOS

# Memory leak test
valgrind --leak-check=full ./test_lseco            # Linux

# Address sanitizer
make CFLAGS="-fsanitize=address -g" && make run-test
```

## Platform-Specific Notes

### Linux
- Full support for all features
- Uses `explicit_bzero()` if glibc ≥ 2.25
- Uses `MADV_DONTDUMP` for core dump prevention
- Full RELRO enabled

### macOS
- Full support for all features
- Uses volatile pointer for secure zeroing
- No `MADV_DONTDUMP` equivalent
- `-bind_at_load` deprecated but functional

### Windows
- Full support via Win32 API
- Uses `VirtualAlloc()` (auto page-aligned)
- Uses `SecureZeroMemory()` (guaranteed)
- Control Flow Guard via `/guard:cf`

## Future Enhancements

### Planned
- [ ] Windows build script (MSVC)
- [ ] CMake build system
- [ ] Python ctypes bindings
- [ ] Node.js N-API bindings
- [ ] Rust FFI bindings
- [ ] Benchmark suite
- [ ] Fuzzing tests (AFL/libFuzzer)

### Considered
- [ ] Encrypted memory content (AES-GCM)
- [ ] Memory pressure handling
- [ ] Windows Credential Manager integration
- [ ] macOS Keychain integration
- [ ] Linux kernel keyring integration

## Contributing

See main README.md for contribution guidelines.

## References

- POSIX mlock: `man 2 mlock`
- POSIX mprotect: `man 2 mprotect`
- Windows VirtualAlloc: [MSDN](https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualalloc)
- GCC Hardening: [Hardening Compiler Options](https://developers.redhat.com/blog/2018/03/21/compiler-and-linker-flags-gcc)
