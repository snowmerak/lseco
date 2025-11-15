#ifndef LSECO_FFI_H
#define LSECO_FFI_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Export symbols for shared library */
#ifdef _WIN32
    #ifdef LSECO_EXPORTS
        #define LSECO_API __declspec(dllexport)
    #else
        #define LSECO_API __declspec(dllimport)
    #endif
#else
    #define LSECO_API __attribute__((visibility("default")))
#endif

/* Error codes (same as secure_memory.h) */
#define LSECO_SUCCESS           0
#define LSECO_ERR_NULL_PTR      -1
#define LSECO_ERR_ALLOC_FAILED  -2
#define LSECO_ERR_LOCK_FAILED   -3
#define LSECO_ERR_PROTECT_FAILED -4
#define LSECO_ERR_INVALID_SIZE  -5

/* Opaque handle for FFI use */
typedef void* lseco_handle_t;

/**
 * @brief Create a secure storage for sensitive data
 * 
 * This function allocates page-aligned, RAM-locked memory for storing
 * sensitive data like passwords, keys, or tokens.
 * 
 * @param size Size in bytes to allocate (must be > 0)
 * @return Handle to secure storage on success, NULL on failure
 * 
 * Example (Go):
 *   handle := C.lseco_create(256)
 *   if handle == nil { panic("failed to create secure storage") }
 *   defer C.lseco_destroy(handle)
 */
LSECO_API lseco_handle_t lseco_create(size_t size);

/**
 * @brief Store sensitive data in secure storage
 * 
 * Temporarily unlocks the memory, writes data, then locks it again.
 * Data will be encrypted in memory and never swapped to disk.
 * 
 * @param handle Valid handle from lseco_create (must not be NULL)
 * @param data Buffer containing data to store (must not be NULL)
 * @param length Length of data in bytes (must be > 0 and <= allocated size)
 * @return LSECO_SUCCESS on success, error code on failure
 * 
 * Example (Go):
 *   password := []byte("secret123")
 *   result := C.lseco_store(handle, unsafe.Pointer(&password[0]), C.size_t(len(password)))
 *   if result != 0 { panic("failed to store data") }
 */
LSECO_API int lseco_store(lseco_handle_t handle, const void* data, size_t length);

/**
 * @brief Retrieve sensitive data from secure storage
 * 
 * Temporarily unlocks the memory, reads data, then locks it again.
 * Caller must provide a buffer large enough to hold the data.
 * 
 * @param handle Valid handle from lseco_create (must not be NULL)
 * @param buffer Output buffer to receive data (must not be NULL)
 * @param length Number of bytes to read (must be > 0 and <= allocated size)
 * @return LSECO_SUCCESS on success, error code on failure
 * 
 * Example (Go):
 *   buffer := make([]byte, 256)
 *   result := C.lseco_retrieve(handle, unsafe.Pointer(&buffer[0]), C.size_t(len(buffer)))
 *   if result != 0 { panic("failed to retrieve data") }
 */
LSECO_API int lseco_retrieve(lseco_handle_t handle, void* buffer, size_t length);

/**
 * @brief Get the size of allocated secure storage
 * 
 * @param handle Valid handle from lseco_create (must not be NULL)
 * @return Size in bytes, or 0 if handle is NULL
 * 
 * Example (Go):
 *   size := C.lseco_get_size(handle)
 */
LSECO_API size_t lseco_get_size(lseco_handle_t handle);

/**
 * @brief Securely destroy secure storage
 * 
 * Zeros all data, unlocks memory, and frees resources.
 * Handle will be invalidated and must not be used after this call.
 * Safe to call with NULL handle (no-op).
 * 
 * IMPORTANT: This function MUST be called to free C-allocated memory.
 * Language GC (Go/PHP/Node) cannot free C memory automatically.
 * 
 * @param handle Handle to destroy (can be NULL)
 * 
 * Example (Go):
 *   defer C.lseco_destroy(handle)
 */
LSECO_API void lseco_destroy(lseco_handle_t handle);

/**
 * @brief Get human-readable error message for error code
 * 
 * @param error_code Error code from lseco functions
 * @return Static string describing the error (never NULL)
 * 
 * Example (Go):
 *   if result != 0 {
 *     msg := C.GoString(C.lseco_error_string(result))
 *     log.Printf("Error: %s", msg)
 *   }
 */
LSECO_API const char* lseco_error_string(int error_code);

/**
 * @brief Get library version string
 * 
 * @return Static string with version (e.g., "1.0.0")
 */
LSECO_API const char* lseco_version(void);

#ifdef __cplusplus
}
#endif

#endif /* LSECO_FFI_H */
