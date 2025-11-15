# Lseco - Secure Memory Library

A hardened C library for storing sensitive data (passwords, keys, tokens) in memory with maximum security.

## 🔐 Security Features

### Memory Protection
- **Page-aligned allocation** via `posix_memalign()` / `VirtualAlloc()`
- **Access control**: Memory is `PROT_NONE` / `PAGE_NOACCESS` by default
- **RAM locking**: Prevents swapping to disk (`mlock` / `VirtualLock`)
- **Secure deletion**: Zeros memory before free (`explicit_bzero` / `SecureZeroMemory`)
- **Core dump prevention**: Uses `MADV_DONTDUMP` on Linux

### Compilation Hardening
- **Stack protector**: `-fstack-protector-all` / `/GS`
- **ASLR support**: `-fPIC` / `/DYNAMICBASE`
- **Control flow integrity**: `-fcf-protection` / `/guard:cf`
- **Full RELRO**: `-Wl,-z,relro,-z,now`

### FFI Safety
- **No crashes**: Never calls `exit()` or `abort()`
- **Input validation**: NULL checks, size bounds
- **Error codes**: Returns integers for all operations
- **Memory management**: Caller must explicitly free

## 📦 Installation

### Build from source

```bash
# Clone repository
git clone https://github.com/snowmerak/lseco.git
cd lseco

# Build library
make

# Run tests
make run-test

# Install system-wide (optional)
sudo make install
```

### Build outputs
- `liblseco.a` - Static library
- `liblseco.so` / `liblseco.dylib` - Shared library

## 🚀 Usage

### C Example

```c
#include "lseco_ffi.h"
#include <stdio.h>
#include <string.h>

int main() {
    // Create secure storage (256 bytes)
    lseco_handle_t handle = lseco_create(256);
    if (handle == NULL) {
        fprintf(stderr, "Failed to create secure storage\n");
        return 1;
    }
    
    // Store sensitive data
    const char* password = "my_secret_password";
    int result = lseco_store(handle, password, strlen(password) + 1);
    if (result != LSECO_SUCCESS) {
        fprintf(stderr, "Store failed: %s\n", lseco_error_string(result));
        lseco_destroy(handle);
        return 1;
    }
    
    // Retrieve data
    char buffer[256] = {0};
    result = lseco_retrieve(handle, buffer, sizeof(buffer));
    if (result != LSECO_SUCCESS) {
        fprintf(stderr, "Retrieve failed: %s\n", lseco_error_string(result));
        lseco_destroy(handle);
        return 1;
    }
    
    printf("Retrieved: %s\n", buffer);
    
    // IMPORTANT: Always destroy to free memory
    lseco_destroy(handle);
    
    return 0;
}
```

### Go Example

```go
package main

/*
#cgo LDFLAGS: -L. -llseco
#include "lseco_ffi.h"
#include <stdlib.h>
*/
import "C"
import (
    "fmt"
    "unsafe"
)

func main() {
    // Create secure storage (256 bytes)
    handle := C.lseco_create(256)
    if handle == nil {
        panic("failed to create secure storage")
    }
    defer C.lseco_destroy(handle)
    
    // Store password
    password := []byte("my_secret_password")
    result := C.lseco_store(
        handle,
        unsafe.Pointer(&password[0]),
        C.size_t(len(password)),
    )
    if result != 0 {
        msg := C.GoString(C.lseco_error_string(result))
        panic(fmt.Sprintf("store failed: %s", msg))
    }
    
    // Retrieve password
    buffer := make([]byte, 256)
    result = C.lseco_retrieve(
        handle,
        unsafe.Pointer(&buffer[0]),
        C.size_t(len(buffer)),
    )
    if result != 0 {
        msg := C.GoString(C.lseco_error_string(result))
        panic(fmt.Sprintf("retrieve failed: %s", msg))
    }
    
    fmt.Printf("Retrieved: %s\n", string(buffer))
}
```

**For comprehensive examples with error handling, see [examples/go/](examples/go/).**

### PHP Example

```php
<?php

$ffi = FFI::cdef("
    typedef void* lseco_handle_t;
    lseco_handle_t lseco_create(size_t size);
    int lseco_store(lseco_handle_t handle, const void* data, size_t length);
    int lseco_retrieve(lseco_handle_t handle, void* buffer, size_t length);
    void lseco_destroy(lseco_handle_t handle);
    const char* lseco_error_string(int error_code);
", "liblseco.so");

// Create secure storage
$handle = $ffi->lseco_create(256);
if ($handle === null) {
    die("Failed to create secure storage\n");
}

// Store password
$password = "my_secret_password";
$result = $ffi->lseco_store($handle, $password, strlen($password) + 1);
if ($result !== 0) {
    $error = $ffi->lseco_error_string($result);
    die("Store failed: $error\n");
}

// Retrieve password
$buffer = FFI::new("char[256]");
$result = $ffi->lseco_retrieve($handle, $buffer, 256);
if ($result !== 0) {
    $error = $ffi->lseco_error_string($result);
    die("Retrieve failed: $error\n");
}

echo "Retrieved: " . FFI::string($buffer) . "\n";

// IMPORTANT: Always destroy
$ffi->lseco_destroy($handle);
```

## 📚 API Reference

### Core Functions

#### `lseco_handle_t lseco_create(size_t size)`
Create secure storage for sensitive data.

- **Parameters**: `size` - bytes to allocate (must be > 0)
- **Returns**: Handle on success, NULL on failure
- **Thread-safe**: Yes

#### `int lseco_store(lseco_handle_t handle, const void* data, size_t length)`
Store data in secure storage.

- **Parameters**: 
  - `handle` - valid handle from `lseco_create()`
  - `data` - buffer containing data (must not be NULL)
  - `length` - data size (must be > 0 and <= allocated size)
- **Returns**: `LSECO_SUCCESS` or error code
- **Thread-safe**: No (requires external synchronization)

#### `int lseco_retrieve(lseco_handle_t handle, void* buffer, size_t length)`
Retrieve data from secure storage.

- **Parameters**:
  - `handle` - valid handle from `lseco_create()`
  - `buffer` - output buffer (must not be NULL)
  - `length` - bytes to read (must be > 0 and <= allocated size)
- **Returns**: `LSECO_SUCCESS` or error code
- **Thread-safe**: No (requires external synchronization)

#### `void lseco_destroy(lseco_handle_t handle)`
Securely destroy storage (zeros memory and frees).

- **Parameters**: `handle` - handle to destroy (can be NULL)
- **Returns**: void
- **Thread-safe**: Yes (safe to call with NULL)

#### `size_t lseco_get_size(lseco_handle_t handle)`
Get allocated storage size.

- **Parameters**: `handle` - valid handle
- **Returns**: Size in bytes, or 0 if NULL
- **Thread-safe**: Yes

### Utility Functions

#### `const char* lseco_error_string(int error_code)`
Get human-readable error message.

- **Parameters**: `error_code` - error from lseco functions
- **Returns**: Static string (never NULL)
- **Thread-safe**: Yes

#### `const char* lseco_version(void)`
Get library version.

- **Returns**: Version string (e.g., "1.0.0")
- **Thread-safe**: Yes

### Error Codes

| Code | Value | Description |
|------|-------|-------------|
| `LSECO_SUCCESS` | 0 | Operation succeeded |
| `LSECO_ERR_NULL_PTR` | -1 | NULL pointer provided |
| `LSECO_ERR_ALLOC_FAILED` | -2 | Memory allocation failed |
| `LSECO_ERR_LOCK_FAILED` | -3 | Failed to lock memory in RAM |
| `LSECO_ERR_PROTECT_FAILED` | -4 | Failed to set memory protection |
| `LSECO_ERR_INVALID_SIZE` | -5 | Invalid size parameter |

## ⚠️ Important Notes

### Memory Management
- **Always call `lseco_destroy()`** - Go/PHP GC cannot free C memory
- **One handle per context** - Do not share handles across threads without synchronization

### Error Handling Examples

#### ❌ Common Mistakes
```c
// Mistake 1: Not checking return values
lseco_store(handle, data, size);  // Ignoring errors!

// Mistake 2: Forgetting to destroy
handle = lseco_create(256);
// ... use it ...
// Forgot to call lseco_destroy(handle) - Memory leak!

// Mistake 3: Using invalid sizes
lseco_create(0);              // Invalid: size must be > 0
lseco_store(handle, data, 0); // Invalid: length must be > 0
```

#### ✅ Correct Usage
```c
// Always check errors
lseco_handle_t handle = lseco_create(256);
if (handle == NULL) {
    fprintf(stderr, "Failed to create storage\n");
    return -1;
}

// Always validate sizes
int result = lseco_store(handle, password, strlen(password) + 1);
if (result != LSECO_SUCCESS) {
    fprintf(stderr, "Store failed: %s\n", lseco_error_string(result));
    lseco_destroy(handle);
    return -1;
}

// Always destroy
lseco_destroy(handle);
```

#### Go Error Handling Pattern
```go
storage, err := NewSecureStorage(256)
if err != nil {
    return fmt.Errorf("create failed: %w", err)
}
defer storage.Destroy() // Guaranteed cleanup

if err := storage.Store(data); err != nil {
    return fmt.Errorf("store failed: %w", err)
}
```

#### PHP Error Handling Pattern
```php
$handle = $ffi->lseco_create(256);
if ($handle === null) {
    throw new Exception("Failed to create storage");
}

try {
    $result = $ffi->lseco_store($handle, $data, strlen($data));
    if ($result !== 0) {
        $error = $ffi->lseco_error_string($result);
        throw new Exception("Store failed: $error");
    }
} finally {
    $ffi->lseco_destroy($handle); // Always cleanup
}
```

### Thread Safety
- `create`, `destroy`, utility functions are thread-safe
- `store`, `retrieve` require external synchronization for same handle

### Platform Support
- **Linux**: Fully supported (all features)
- **macOS**: Fully supported (all features)
- **Windows**: Fully supported (uses `VirtualAlloc`, `SecureZeroMemory`)

### Performance
- Page-aligned allocation may use more memory than requested
- `mlock` has system limits (check `ulimit -l` on Linux)
- Memory access requires permission changes (2 syscalls per operation)

## 🛠️ Build Options

### Debug Build
```bash
make CFLAGS="-Wall -Wextra -g -O0 -fPIC -fstack-protector-all"
```

### Static Analysis
```bash
# Clang static analyzer
scan-build make

# Cppcheck
cppcheck --enable=all --inconclusive *.c
```

### Address Sanitizer
```bash
make CFLAGS="-Wall -Wextra -g -O1 -fPIC -fsanitize=address"
make run-test
```

## 📄 License

See [LICENSE](LICENSE) file for details.

## 🤝 Contributing

Contributions welcome! Please ensure:
- All tests pass (`make run-test`)
- Code follows security guidelines
- No new compiler warnings

## 📞 Support

- Issues: [GitHub Issues](https://github.com/snowmerak/lseco/issues)
- Security: Report privately to maintainers

## 🔍 Security Audit

This library implements defense-in-depth:
1. **Memory isolation** (page boundaries + protection)
2. **RAM locking** (no disk leakage)
3. **Secure deletion** (compiler-resistant zeroing)
4. **Hardened compilation** (CFI, RELRO, stack canaries)
5. **Input validation** (NULL checks, bounds)

For production use, consider third-party security audit.
