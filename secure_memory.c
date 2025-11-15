#define _GNU_SOURCE
#include "secure_memory.h"
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <sys/mman.h>
    #include <unistd.h>
#endif

/* Internal structure */
struct secure_memory_t {
    void* data;
    size_t size;
    size_t page_size;
#ifdef _WIN32
    HANDLE process_handle;
#endif
};

/* Get system page size */
static size_t get_page_size(void) {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
#else
    return (size_t)sysconf(_SC_PAGESIZE);
#endif
}

/* Secure zero memory (compiler-optimization-safe) */
static void secure_zero(void* ptr, size_t size) {
#ifdef _WIN32
    SecureZeroMemory(ptr, size);
#else
    #ifdef __STDC_LIB_EXT1__
        memset_s(ptr, size, 0, size);
    #elif defined(__GLIBC__) && __GLIBC__ >= 2 && __GLIBC_MINOR__ >= 25
        explicit_bzero(ptr, size);
    #else
        volatile unsigned char* p = (volatile unsigned char*)ptr;
        while (size--) {
            *p++ = 0;
        }
    #endif
#endif
}

/* Set memory protection */
static int set_memory_protection(void* addr, size_t size, int allow_access) {
#ifdef _WIN32
    DWORD old_protect;
    DWORD new_protect = allow_access ? PAGE_READWRITE : PAGE_NOACCESS;
    if (!VirtualProtect(addr, size, new_protect, &old_protect)) {
        return SECURE_ERR_PROTECT_FAILED;
    }
#else
    int prot = allow_access ? (PROT_READ | PROT_WRITE) : PROT_NONE;
    if (mprotect(addr, size, prot) != 0) {
        return SECURE_ERR_PROTECT_FAILED;
    }
#endif
    return SECURE_SUCCESS;
}

/* Lock memory in RAM */
static int lock_memory(void* addr, size_t size) {
#ifdef _WIN32
    if (!VirtualLock(addr, size)) {
        return SECURE_ERR_LOCK_FAILED;
    }
#else
    if (mlock(addr, size) != 0) {
        return SECURE_ERR_LOCK_FAILED;
    }
    
    /* Prevent core dump on Linux */
    #ifdef __linux__
        #ifdef MADV_DONTDUMP
            madvise(addr, size, MADV_DONTDUMP);
        #endif
    #endif
#endif
    return SECURE_SUCCESS;
}

/* Unlock memory */
static void unlock_memory(void* addr, size_t size) {
#ifdef _WIN32
    VirtualUnlock(addr, size);
#else
    munlock(addr, size);
#endif
}

int secure_memory_create(secure_memory_t** handle, size_t size) {
    /* Input validation */
    if (handle == NULL) {
        return SECURE_ERR_NULL_PTR;
    }
    if (size == 0) {
        return SECURE_ERR_INVALID_SIZE;
    }
    
    /* Allocate handle structure */
    secure_memory_t* mem = (secure_memory_t*)malloc(sizeof(secure_memory_t));
    if (mem == NULL) {
        return SECURE_ERR_ALLOC_FAILED;
    }
    
    mem->size = size;
    mem->page_size = get_page_size();
    
    /* Round up size to page boundary */
    size_t aligned_size = ((size + mem->page_size - 1) / mem->page_size) * mem->page_size;
    
    /* Allocate page-aligned memory */
#ifdef _WIN32
    mem->process_handle = GetCurrentProcess();
    mem->data = VirtualAlloc(NULL, aligned_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (mem->data == NULL) {
        free(mem);
        return SECURE_ERR_ALLOC_FAILED;
    }
#else
    if (posix_memalign(&mem->data, mem->page_size, aligned_size) != 0) {
        free(mem);
        return SECURE_ERR_ALLOC_FAILED;
    }
#endif
    
    /* Lock memory in RAM */
    int lock_result = lock_memory(mem->data, aligned_size);
    if (lock_result != SECURE_SUCCESS) {
#ifdef _WIN32
        VirtualFree(mem->data, 0, MEM_RELEASE);
#else
        free(mem->data);
#endif
        free(mem);
        return lock_result;
    }
    
    /* Set memory to NOACCESS */
    int protect_result = set_memory_protection(mem->data, aligned_size, 0);
    if (protect_result != SECURE_SUCCESS) {
        unlock_memory(mem->data, aligned_size);
#ifdef _WIN32
        VirtualFree(mem->data, 0, MEM_RELEASE);
#else
        free(mem->data);
#endif
        free(mem);
        return protect_result;
    }
    
    *handle = mem;
    return SECURE_SUCCESS;
}

int secure_memory_write(secure_memory_t* handle, const void* data, size_t length) {
    /* Input validation */
    if (handle == NULL || data == NULL) {
        return SECURE_ERR_NULL_PTR;
    }
    if (length == 0 || length > handle->size) {
        return SECURE_ERR_INVALID_SIZE;
    }
    
    size_t aligned_size = ((handle->size + handle->page_size - 1) / handle->page_size) * handle->page_size;
    
    /* Grant READWRITE permission */
    int result = set_memory_protection(handle->data, aligned_size, 1);
    if (result != SECURE_SUCCESS) {
        return result;
    }
    
    /* Copy data */
    memcpy(handle->data, data, length);
    
    /* Revoke access */
    result = set_memory_protection(handle->data, aligned_size, 0);
    if (result != SECURE_SUCCESS) {
        return result;
    }
    
    return SECURE_SUCCESS;
}

int secure_memory_read(const secure_memory_t* handle, void* buffer, size_t length) {
    /* Input validation */
    if (handle == NULL || buffer == NULL) {
        return SECURE_ERR_NULL_PTR;
    }
    if (length == 0 || length > handle->size) {
        return SECURE_ERR_INVALID_SIZE;
    }
    
    size_t aligned_size = ((handle->size + handle->page_size - 1) / handle->page_size) * handle->page_size;
    
    /* Grant READ permission - need to cast away const for mprotect */
    secure_memory_t* mutable_handle = (secure_memory_t*)handle;
    int result = set_memory_protection(mutable_handle->data, aligned_size, 1);
    if (result != SECURE_SUCCESS) {
        return result;
    }
    
    /* Copy data out */
    memcpy(buffer, handle->data, length);
    
    /* Revoke access */
    result = set_memory_protection(mutable_handle->data, aligned_size, 0);
    if (result != SECURE_SUCCESS) {
        return result;
    }
    
    return SECURE_SUCCESS;
}

void secure_memory_destroy(secure_memory_t** handle) {
    if (handle == NULL || *handle == NULL) {
        return;
    }
    
    secure_memory_t* mem = *handle;
    size_t aligned_size = ((mem->size + mem->page_size - 1) / mem->page_size) * mem->page_size;
    
    /* Grant access to zero the memory */
    set_memory_protection(mem->data, aligned_size, 1);
    
    /* Securely zero memory */
    secure_zero(mem->data, aligned_size);
    
    /* Unlock memory */
    unlock_memory(mem->data, aligned_size);
    
    /* Free memory */
#ifdef _WIN32
    VirtualFree(mem->data, 0, MEM_RELEASE);
#else
    free(mem->data);
#endif
    
    /* Free handle */
    free(mem);
    *handle = NULL;
}

size_t secure_memory_get_size(const secure_memory_t* handle) {
    if (handle == NULL) {
        return 0;
    }
    return handle->size;
}
