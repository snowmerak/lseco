# Example: Go FFI Integration

This example demonstrates how to use Lseco from Go using CGo, including both success and failure cases.

## Setup

1. Build the Lseco library:
```bash
cd ../
make
```

2. Run the Go example:
```bash
go run main.go
```

## Features Demonstrated

### Success Cases
- ✅ Creating secure storage
- ✅ Storing and retrieving passwords
- ✅ Storing and retrieving API keys
- ✅ Proper cleanup with defer

### Failure Cases (Error Handling)
- ✅ Creating storage with invalid size (0)
- ✅ Storing empty data
- ✅ Storing data larger than allocated size
- ✅ Retrieving with invalid length (0)
- ✅ Retrieving more than allocated size
- ✅ Safe multiple destroy calls

### Edge Cases
- ✅ Minimum valid size (1 byte)
- ✅ Large allocations (1 MB)
- ✅ Binary data with null bytes
- ✅ Overwriting data multiple times

## Code Structure

### SecureStorage Wrapper
```go
type SecureStorage struct {
    handle C.lseco_handle_t
    size   int
}
```

A Go-friendly wrapper around the C handle that:
- Validates input before calling C functions
- Converts C error codes to Go errors
- Provides idiomatic Go API

### Main Demonstrations

1. **demonstrateSuccessCases()** - Basic usage patterns
2. **demonstrateFailureCases()** - Error handling examples
3. **demonstrateEdgeCases()** - Boundary conditions

## Example Output

```
==================================================
  Lseco Go Example - Comprehensive Test
==================================================

Library version: 1.0.0

=== Success Cases ===

✓ Storing password: SuperSecret123!
✓ Retrieved password: SuperSecret123!
✓ Storing API key: sk-1234567890abcdef
✓ Retrieved API key: sk-1234567890abcdef

=== Failure Cases (Error Handling) ===

1. Creating storage with size 0:
   ✗ Expected error: failed to create secure storage

2. Storing empty data:
   ✗ Expected error: data cannot be empty

3. Storing data larger than storage size:
   ✗ Expected error: data size 48 exceeds storage size 16

4. Retrieving with length 0:
   ✗ Expected error: invalid length 0 (max: 256)

5. Retrieving more than storage size:
   ✗ Expected error: invalid length 512 (max: 256)

6. Using storage after destroy:
   ✓ Destroy is safe to call multiple times

=== Edge Cases ===

1. Creating storage with size 1:
   ✓ Success! Can create 1-byte storage
   ✓ Stored 1 byte
   ✓ Retrieved: 0x42

2. Creating large storage (1 MB):
   ✓ Success! Allocated 1 MB

3. Storing binary data with null bytes:
   ✓ Stored binary data with nulls
   ✓ Retrieved: [0 1 2 0 255 254 0]

4. Overwriting data multiple times:
   ✓ Write 1: Data version 1
   ✓ Write 2: Data version 2
   ✓ Write 3: Data version 3
```

## Complete Code Example

```go
package main

/*
#cgo CFLAGS: -I../../
#cgo LDFLAGS: -L../../ -llseco
#include "lseco_ffi.h"
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"unsafe"
)

// SecureStorage wraps lseco_handle_t with Go-friendly interface
type SecureStorage struct {
	handle C.lseco_handle_t
	size   int
}

// NewSecureStorage creates a new secure storage
func NewSecureStorage(size int) (*SecureStorage, error) {
	handle := C.lseco_create(C.size_t(size))
	if handle == nil {
		return nil, fmt.Errorf("failed to create secure storage")
	}

	return &SecureStorage{
		handle: handle,
		size:   size,
	}, nil
}

// Store stores data in secure memory
func (s *SecureStorage) Store(data []byte) error {
	if len(data) == 0 {
		return fmt.Errorf("data cannot be empty")
	}
	if len(data) > s.size {
		return fmt.Errorf("data size %d exceeds storage size %d", len(data), s.size)
	}

	result := C.lseco_store(
		s.handle,
		unsafe.Pointer(&data[0]),
		C.size_t(len(data)),
	)

	if result != C.LSECO_SUCCESS {
		msg := C.GoString(C.lseco_error_string(result))
		return fmt.Errorf("store failed: %s", msg)
	}

	return nil
}

// Retrieve retrieves data from secure memory
func (s *SecureStorage) Retrieve(length int) ([]byte, error) {
	if length == 0 || length > s.size {
		return nil, fmt.Errorf("invalid length %d (max: %d)", length, s.size)
	}

	buffer := make([]byte, length)
	result := C.lseco_retrieve(
		s.handle,
		unsafe.Pointer(&buffer[0]),
		C.size_t(length),
	)

	if result != C.LSECO_SUCCESS {
		msg := C.GoString(C.lseco_error_string(result))
		return nil, fmt.Errorf("retrieve failed: %s", msg)
	}

	return buffer, nil
}

// Destroy securely destroys the storage
func (s *SecureStorage) Destroy() {
	if s.handle != nil {
		C.lseco_destroy(s.handle)
		s.handle = nil
	}
}

func main() {
	// Create secure storage
	storage, err := NewSecureStorage(256)
	if err != nil {
		panic(err)
	}
	defer storage.Destroy()

	// Store password
	password := []byte("SuperSecret123!")
	if err := storage.Store(password); err != nil {
		panic(err)
	}

	// Retrieve password
	retrieved, err := storage.Retrieve(len(password))
	if err != nil {
		panic(err)
	}
	fmt.Printf("Retrieved: %s\n", string(retrieved))
}
```

## Error Handling Best Practices

### 1. Always Check Errors
```go
storage, err := NewSecureStorage(256)
if err != nil {
    log.Printf("Failed to create storage: %v", err)
    return err
}
```

### 2. Use Defer for Cleanup
```go
storage, err := NewSecureStorage(256)
if err != nil {
    return err
}
defer storage.Destroy() // Always cleanup
```

### 3. Validate Before Operations
```go
func (s *SecureStorage) Store(data []byte) error {
    if len(data) == 0 {
        return fmt.Errorf("data cannot be empty")
    }
    if len(data) > s.size {
        return fmt.Errorf("data too large")
    }
    // ... proceed with C call
}
```

### 4. Handle C Error Codes
```go
result := C.lseco_store(s.handle, ptr, size)
if result != C.LSECO_SUCCESS {
    msg := C.GoString(C.lseco_error_string(result))
    return fmt.Errorf("operation failed: %s", msg)
}
```
package main

/*
#cgo CFLAGS: -I../
#cgo LDFLAGS: -L../ -llseco
#include "lseco_ffi.h"
#include <stdlib.h>
*/
import "C"
import (
	"fmt"
	"unsafe"
)

// SecureStorage wraps lseco_handle_t with Go-friendly interface
type SecureStorage struct {
	handle C.lseco_handle_t
	size   int
}

// NewSecureStorage creates a new secure storage
func NewSecureStorage(size int) (*SecureStorage, error) {
	handle := C.lseco_create(C.size_t(size))
	if handle == nil {
		return nil, fmt.Errorf("failed to create secure storage")
	}
	
	return &SecureStorage{
		handle: handle,
		size:   size,
	}, nil
}

// Store stores data in secure memory
func (s *SecureStorage) Store(data []byte) error {
	if len(data) == 0 {
		return fmt.Errorf("data cannot be empty")
	}
	if len(data) > s.size {
		return fmt.Errorf("data size %d exceeds storage size %d", len(data), s.size)
	}
	
	result := C.lseco_store(
		s.handle,
		unsafe.Pointer(&data[0]),
		C.size_t(len(data)),
	)
	
	if result != C.LSECO_SUCCESS {
		msg := C.GoString(C.lseco_error_string(result))
		return fmt.Errorf("store failed: %s", msg)
	}
	
	return nil
}

// Retrieve retrieves data from secure memory
func (s *SecureStorage) Retrieve(length int) ([]byte, error) {
	if length == 0 || length > s.size {
		return nil, fmt.Errorf("invalid length %d (max: %d)", length, s.size)
	}
	
	buffer := make([]byte, length)
	result := C.lseco_retrieve(
		s.handle,
		unsafe.Pointer(&buffer[0]),
		C.size_t(length),
	)
	
	if result != C.LSECO_SUCCESS {
		msg := C.GoString(C.lseco_error_string(result))
		return nil, fmt.Errorf("retrieve failed: %s", msg)
	}
	
	return buffer, nil
}

// Destroy securely destroys the storage
func (s *SecureStorage) Destroy() {
	if s.handle != nil {
		C.lseco_destroy(s.handle)
		s.handle = nil
	}
}

func main() {
	fmt.Println("Lseco Go Example")
	fmt.Println("================")
	
	// Print version
	version := C.GoString(C.lseco_version())
	fmt.Printf("Library version: %s\n\n", version)
	
	// Create secure storage
	storage, err := NewSecureStorage(256)
	if err != nil {
		panic(err)
	}
	defer storage.Destroy()
	
	// Store password
	password := []byte("SuperSecret123!")
	fmt.Printf("Storing password: %s\n", string(password))
	if err := storage.Store(password); err != nil {
		panic(err)
	}
	
	// Retrieve password
	retrieved, err := storage.Retrieve(len(password))
	if err != nil {
		panic(err)
	}
	fmt.Printf("Retrieved password: %s\n", string(retrieved))
	
	// Store API key
	apiKey := []byte("sk-1234567890abcdef")
	fmt.Printf("\nStoring API key: %s\n", string(apiKey))
	if err := storage.Store(apiKey); err != nil {
		panic(err)
	}
	
	// Retrieve API key
	retrieved, err = storage.Retrieve(len(apiKey))
	if err != nil {
		panic(err)
	}
	fmt.Printf("Retrieved API key: %s\n", string(retrieved))
	
	fmt.Println("\n✓ All operations completed successfully")
}
```

## Build and Run

```bash
# Make sure library is built first
cd ../..
make

# Run example with all demonstrations
cd examples/go
DYLD_LIBRARY_PATH=../../ go run main.go  # macOS
# or
LD_LIBRARY_PATH=../../ go run main.go     # Linux
```

## Common Pitfalls

### ❌ Don't: Forget to call Destroy
```go
storage, _ := NewSecureStorage(256)
// Memory leak! C memory not freed
```

### ✅ Do: Use defer
```go
storage, err := NewSecureStorage(256)
if err != nil {
    return err
}
defer storage.Destroy() // Guaranteed cleanup
```

### ❌ Don't: Use after Destroy
```go
storage.Destroy()
storage.Store(data) // Undefined behavior!
```

### ✅ Do: Check handle validity
```go
func (s *SecureStorage) Store(data []byte) error {
    if s.handle == nil {
        return fmt.Errorf("storage already destroyed")
    }
    // ... proceed
}
```

## Notes

- Always call `Destroy()` to free C-allocated memory
- Use `defer storage.Destroy()` to ensure cleanup
- Go's GC cannot free C memory automatically
- For production, add proper error handling and logging
- Thread safety requires external synchronization
