#define LSECO_EXPORTS
#include "lseco_ffi.h"
#include "secure_memory.h"
#include <stdlib.h>

#define LSECO_VERSION "1.0.0"

/* FFI wrapper: Create secure storage */
LSECO_API lseco_handle_t lseco_create(size_t size) {
    /* Input validation */
    if (size == 0) {
        return NULL;
    }
    
    secure_memory_t* handle = NULL;
    int result = secure_memory_create(&handle, size);
    
    if (result != SECURE_SUCCESS) {
        return NULL;
    }
    
    return (lseco_handle_t)handle;
}

/* FFI wrapper: Store data */
LSECO_API int lseco_store(lseco_handle_t handle, const void* data, size_t length) {
    /* Input validation - prevent DoS, never call exit/abort */
    if (handle == NULL) {
        return LSECO_ERR_NULL_PTR;
    }
    if (data == NULL) {
        return LSECO_ERR_NULL_PTR;
    }
    if (length == 0) {
        return LSECO_ERR_INVALID_SIZE;
    }
    
    secure_memory_t* mem = (secure_memory_t*)handle;
    return secure_memory_write(mem, data, length);
}

/* FFI wrapper: Retrieve data */
LSECO_API int lseco_retrieve(lseco_handle_t handle, void* buffer, size_t length) {
    /* Input validation */
    if (handle == NULL) {
        return LSECO_ERR_NULL_PTR;
    }
    if (buffer == NULL) {
        return LSECO_ERR_NULL_PTR;
    }
    if (length == 0) {
        return LSECO_ERR_INVALID_SIZE;
    }
    
    secure_memory_t* mem = (secure_memory_t*)handle;
    return secure_memory_read(mem, buffer, length);
}

/* FFI wrapper: Get size */
LSECO_API size_t lseco_get_size(lseco_handle_t handle) {
    /* NULL check */
    if (handle == NULL) {
        return 0;
    }
    
    secure_memory_t* mem = (secure_memory_t*)handle;
    return secure_memory_get_size(mem);
}

/* FFI wrapper: Destroy (memory management responsibility) */
LSECO_API void lseco_destroy(lseco_handle_t handle) {
    /* Safe to call with NULL */
    if (handle == NULL) {
        return;
    }
    
    secure_memory_t* mem = (secure_memory_t*)handle;
    secure_memory_destroy(&mem);
}

/* FFI utility: Error string */
LSECO_API const char* lseco_error_string(int error_code) {
    switch (error_code) {
        case LSECO_SUCCESS:
            return "Success";
        case LSECO_ERR_NULL_PTR:
            return "NULL pointer provided";
        case LSECO_ERR_ALLOC_FAILED:
            return "Memory allocation failed";
        case LSECO_ERR_LOCK_FAILED:
            return "Failed to lock memory in RAM";
        case LSECO_ERR_PROTECT_FAILED:
            return "Failed to set memory protection";
        case LSECO_ERR_INVALID_SIZE:
            return "Invalid size parameter";
        default:
            return "Unknown error";
    }
}

/* FFI utility: Version */
LSECO_API const char* lseco_version(void) {
    return LSECO_VERSION;
}
