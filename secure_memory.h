#ifndef SECURE_MEMORY_H
#define SECURE_MEMORY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Error codes */
#define SECURE_SUCCESS           0
#define SECURE_ERR_NULL_PTR      -1
#define SECURE_ERR_ALLOC_FAILED  -2
#define SECURE_ERR_LOCK_FAILED   -3
#define SECURE_ERR_PROTECT_FAILED -4
#define SECURE_ERR_INVALID_SIZE  -5

/* Opaque handle for secure memory */
typedef struct secure_memory_t secure_memory_t;

/**
 * @brief Create a secure memory region
 * 
 * Allocates page-aligned memory, locks it in RAM, and sets it to NOACCESS.
 * 
 * @param handle Pointer to store the created handle
 * @param size Size of memory to allocate (must be > 0)
 * @return SECURE_SUCCESS on success, error code otherwise
 */
int secure_memory_create(secure_memory_t** handle, size_t size);

/**
 * @brief Write data to secure memory
 * 
 * Temporarily grants READWRITE permission, copies data, then revokes access.
 * 
 * @param handle Valid secure memory handle (must not be NULL)
 * @param data Data to write (must not be NULL)
 * @param length Length of data (must be <= allocated size)
 * @return SECURE_SUCCESS on success, error code otherwise
 */
int secure_memory_write(secure_memory_t* handle, const void* data, size_t length);

/**
 * @brief Read data from secure memory
 * 
 * Temporarily grants READ permission, copies data out, then revokes access.
 * 
 * @param handle Valid secure memory handle (must not be NULL)
 * @param buffer Output buffer (must not be NULL)
 * @param length Length to read (must be <= allocated size)
 * @return SECURE_SUCCESS on success, error code otherwise
 */
int secure_memory_read(const secure_memory_t* handle, void* buffer, size_t length);

/**
 * @brief Securely destroy secure memory
 * 
 * Zeros the memory, unlocks it, and frees it.
 * Safe to call with NULL handle.
 * 
 * @param handle Pointer to secure memory handle (will be set to NULL)
 */
void secure_memory_destroy(secure_memory_t** handle);

/**
 * @brief Get the size of allocated secure memory
 * 
 * @param handle Valid secure memory handle (must not be NULL)
 * @return Size in bytes, or 0 if handle is NULL
 */
size_t secure_memory_get_size(const secure_memory_t* handle);

#ifdef __cplusplus
}
#endif

#endif /* SECURE_MEMORY_H */
