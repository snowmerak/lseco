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
	"strings"
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

func demonstrateSuccessCases() {
	fmt.Println("=== Success Cases ===")
	fmt.Println()

	// Create secure storage
	storage, err := NewSecureStorage(256)
	if err != nil {
		panic(err)
	}
	defer storage.Destroy()

	// Store password
	password := []byte("SuperSecret123!")
	fmt.Printf("✓ Storing password: %s\n", string(password))
	if err := storage.Store(password); err != nil {
		panic(err)
	}

	// Retrieve password
	retrieved, err := storage.Retrieve(len(password))
	if err != nil {
		panic(err)
	}
	fmt.Printf("✓ Retrieved password: %s\n", string(retrieved))

	// Store API key
	apiKey := []byte("sk-1234567890abcdef")
	fmt.Printf("✓ Storing API key: %s\n", string(apiKey))
	if err := storage.Store(apiKey); err != nil {
		panic(err)
	}

	// Retrieve API key
	retrieved, err = storage.Retrieve(len(apiKey))
	if err != nil {
		panic(err)
	}
	fmt.Printf("✓ Retrieved API key: %s\n\n", string(retrieved))
}

func demonstrateFailureCases() {
	fmt.Println("=== Failure Cases (Error Handling) ===")
	fmt.Println()

	// Case 1: Create with invalid size (0)
	fmt.Println("1. Creating storage with size 0:")
	storage, err := NewSecureStorage(0)
	if err != nil {
		fmt.Printf("   ✗ Expected error: %v\n\n", err)
	} else {
		fmt.Println("   ✗ Should have failed!")
		storage.Destroy()
	}

	// Case 2: Store empty data
	fmt.Println("2. Storing empty data:")
	storage, err = NewSecureStorage(256)
	if err != nil {
		panic(err)
	}
	defer storage.Destroy()

	err = storage.Store([]byte{})
	if err != nil {
		fmt.Printf("   ✗ Expected error: %v\n\n", err)
	} else {
		fmt.Println("   ✗ Should have failed!")
	}

	// Case 3: Store data larger than allocated size
	fmt.Println("3. Storing data larger than storage size:")
	smallStorage, err := NewSecureStorage(16)
	if err != nil {
		panic(err)
	}
	defer smallStorage.Destroy()

	largeData := []byte("This is a very long string that exceeds 16 bytes")
	err = smallStorage.Store(largeData)
	if err != nil {
		fmt.Printf("   ✗ Expected error: %v\n\n", err)
	} else {
		fmt.Println("   ✗ Should have failed!")
	}

	// Case 4: Retrieve with invalid length (0)
	fmt.Println("4. Retrieving with length 0:")
	_, err = storage.Retrieve(0)
	if err != nil {
		fmt.Printf("   ✗ Expected error: %v\n\n", err)
	} else {
		fmt.Println("   ✗ Should have failed!")
	}

	// Case 5: Retrieve more than allocated size
	fmt.Println("5. Retrieving more than storage size:")
	_, err = storage.Retrieve(512) // storage is 256 bytes
	if err != nil {
		fmt.Printf("   ✗ Expected error: %v\n\n", err)
	} else {
		fmt.Println("   ✗ Should have failed!")
	}

	// Case 6: Using destroyed handle (safe but returns error)
	fmt.Println("6. Using storage after destroy:")
	tempStorage, err := NewSecureStorage(64)
	if err != nil {
		panic(err)
	}
	tempStorage.Destroy()

	// This should be safe but demonstrate the error
	fmt.Println("   ✓ Destroy is safe to call multiple times")
	tempStorage.Destroy() // Safe to call again
	fmt.Println()
}

func demonstrateEdgeCases() {
	fmt.Println("=== Edge Cases ===")
	fmt.Println()

	// Case 1: Minimum valid size
	fmt.Println("1. Creating storage with size 1:")
	storage, err := NewSecureStorage(1)
	if err != nil {
		fmt.Printf("   ✗ Error: %v\n", err)
	} else {
		fmt.Println("   ✓ Success! Can create 1-byte storage")

		// Store and retrieve single byte
		data := []byte{0x42}
		if err := storage.Store(data); err != nil {
			fmt.Printf("   ✗ Store error: %v\n", err)
		} else {
			fmt.Println("   ✓ Stored 1 byte")
		}

		retrieved, err := storage.Retrieve(1)
		if err != nil {
			fmt.Printf("   ✗ Retrieve error: %v\n", err)
		} else {
			fmt.Printf("   ✓ Retrieved: 0x%02X\n", retrieved[0])
		}

		storage.Destroy()
	}
	fmt.Println()

	// Case 2: Large size allocation
	fmt.Println("2. Creating large storage (1 MB):")
	largeStorage, err := NewSecureStorage(1024 * 1024)
	if err != nil {
		fmt.Printf("   ✗ Error: %v\n", err)
	} else {
		fmt.Println("   ✓ Success! Allocated 1 MB")
		largeStorage.Destroy()
	}
	fmt.Println()

	// Case 3: Binary data with null bytes
	fmt.Println("3. Storing binary data with null bytes:")
	storage, err = NewSecureStorage(256)
	if err != nil {
		panic(err)
	}
	defer storage.Destroy()

	binaryData := []byte{0x00, 0x01, 0x02, 0x00, 0xFF, 0xFE, 0x00}
	if err := storage.Store(binaryData); err != nil {
		fmt.Printf("   ✗ Error: %v\n", err)
	} else {
		fmt.Println("   ✓ Stored binary data with nulls")

		retrieved, err := storage.Retrieve(len(binaryData))
		if err != nil {
			fmt.Printf("   ✗ Retrieve error: %v\n", err)
		} else {
			fmt.Printf("   ✓ Retrieved: %v\n", retrieved)
		}
	}
	fmt.Println()

	// Case 4: Overwriting existing data
	fmt.Println("4. Overwriting data multiple times:")
	storage2, err := NewSecureStorage(64)
	if err != nil {
		panic(err)
	}
	defer storage2.Destroy()

	for i := 1; i <= 3; i++ {
		data := []byte(fmt.Sprintf("Data version %d", i))
		if err := storage2.Store(data); err != nil {
			fmt.Printf("   ✗ Write %d failed: %v\n", i, err)
		} else {
			retrieved, _ := storage2.Retrieve(len(data))
			fmt.Printf("   ✓ Write %d: %s\n", i, string(retrieved))
		}
	}
	fmt.Println()
}

func main() {
	fmt.Println("\n" + strings.Repeat("=", 50))
	fmt.Println("  Lseco Go Example - Comprehensive Test")
	fmt.Println(strings.Repeat("=", 50) + "\n")

	// Print version
	version := C.GoString(C.lseco_version())
	fmt.Printf("Library version: %s\n\n", version)

	// Demonstrate different scenarios
	demonstrateSuccessCases()
	demonstrateFailureCases()
	demonstrateEdgeCases()

	fmt.Println(strings.Repeat("=", 50))
	fmt.Println("✓ All demonstrations completed successfully!")
	fmt.Println(strings.Repeat("=", 50) + "\n")
}
