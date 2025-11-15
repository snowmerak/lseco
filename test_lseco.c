#include "lseco_ffi.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

void test_version() {
    printf("Testing lseco_version()... ");
    const char* version = lseco_version();
    assert(version != NULL);
    assert(strlen(version) > 0);
    printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET " (version: %s)\n", version);
}

void test_error_strings() {
    printf("Testing lseco_error_string()... ");
    const char* msg = lseco_error_string(LSECO_SUCCESS);
    assert(msg != NULL);
    assert(strcmp(msg, "Success") == 0);
    
    msg = lseco_error_string(LSECO_ERR_NULL_PTR);
    assert(msg != NULL);
    assert(strlen(msg) > 0);
    
    printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET "\n");
}

void test_create_destroy() {
    printf("Testing lseco_create() and lseco_destroy()... ");
    
    /* Test invalid size */
    lseco_handle_t handle = lseco_create(0);
    assert(handle == NULL);
    
    /* Test valid creation */
    handle = lseco_create(256);
    assert(handle != NULL);
    
    /* Test get_size */
    size_t size = lseco_get_size(handle);
    assert(size == 256);
    
    /* Test destroy */
    lseco_destroy(handle);
    
    /* Test destroy with NULL (should not crash) */
    lseco_destroy(NULL);
    
    printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET "\n");
}

void test_store_retrieve() {
    printf("Testing lseco_store() and lseco_retrieve()... ");
    
    lseco_handle_t handle = lseco_create(256);
    assert(handle != NULL);
    
    /* Test NULL pointer validation */
    int result = lseco_store(NULL, "test", 4);
    assert(result == LSECO_ERR_NULL_PTR);
    
    result = lseco_store(handle, NULL, 4);
    assert(result == LSECO_ERR_NULL_PTR);
    
    /* Test invalid size */
    result = lseco_store(handle, "test", 0);
    assert(result == LSECO_ERR_INVALID_SIZE);
    
    /* Test valid store */
    const char* secret = "This is a secret password!";
    result = lseco_store(handle, secret, strlen(secret) + 1);
    assert(result == LSECO_SUCCESS);
    
    /* Test retrieve with NULL validation */
    char buffer[256] = {0};
    result = lseco_retrieve(NULL, buffer, sizeof(buffer));
    assert(result == LSECO_ERR_NULL_PTR);
    
    result = lseco_retrieve(handle, NULL, sizeof(buffer));
    assert(result == LSECO_ERR_NULL_PTR);
    
    /* Test valid retrieve */
    result = lseco_retrieve(handle, buffer, strlen(secret) + 1);
    assert(result == LSECO_SUCCESS);
    assert(strcmp(buffer, secret) == 0);
    
    lseco_destroy(handle);
    
    printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET "\n");
}

void test_size_limits() {
    printf("Testing size limit enforcement... ");
    
    lseco_handle_t handle = lseco_create(16);
    assert(handle != NULL);
    
    /* Test storing data larger than allocated size */
    char large_data[32] = "This data is too large!";
    int result = lseco_store(handle, large_data, sizeof(large_data));
    assert(result == LSECO_ERR_INVALID_SIZE);
    
    /* Test retrieving more than allocated size */
    char buffer[32];
    result = lseco_retrieve(handle, buffer, sizeof(buffer));
    assert(result == LSECO_ERR_INVALID_SIZE);
    
    /* Test valid size */
    result = lseco_store(handle, "small", 6);
    assert(result == LSECO_SUCCESS);
    
    lseco_destroy(handle);
    
    printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET "\n");
}

void test_multiple_operations() {
    printf("Testing multiple store/retrieve operations... ");
    
    lseco_handle_t handle = lseco_create(1024);
    assert(handle != NULL);
    
    /* Store and retrieve multiple times */
    const char* data1 = "First secret";
    const char* data2 = "Second secret is longer";
    const char* data3 = "Third";
    char buffer[1024];
    
    int result = lseco_store(handle, data1, strlen(data1) + 1);
    assert(result == LSECO_SUCCESS);
    
    result = lseco_retrieve(handle, buffer, strlen(data1) + 1);
    assert(result == LSECO_SUCCESS);
    assert(strcmp(buffer, data1) == 0);
    
    result = lseco_store(handle, data2, strlen(data2) + 1);
    assert(result == LSECO_SUCCESS);
    
    result = lseco_retrieve(handle, buffer, strlen(data2) + 1);
    assert(result == LSECO_SUCCESS);
    assert(strcmp(buffer, data2) == 0);
    
    result = lseco_store(handle, data3, strlen(data3) + 1);
    assert(result == LSECO_SUCCESS);
    
    result = lseco_retrieve(handle, buffer, strlen(data3) + 1);
    assert(result == LSECO_SUCCESS);
    assert(strcmp(buffer, data3) == 0);
    
    lseco_destroy(handle);
    
    printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET "\n");
}

void test_binary_data() {
    printf("Testing binary data storage... ");
    
    lseco_handle_t handle = lseco_create(256);
    assert(handle != NULL);
    
    /* Create binary data with null bytes */
    unsigned char binary_data[16] = {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
    };
    
    int result = lseco_store(handle, binary_data, sizeof(binary_data));
    assert(result == LSECO_SUCCESS);
    
    unsigned char buffer[16];
    result = lseco_retrieve(handle, buffer, sizeof(buffer));
    assert(result == LSECO_SUCCESS);
    
    /* Verify binary data integrity */
    for (size_t i = 0; i < sizeof(binary_data); i++) {
        assert(buffer[i] == binary_data[i]);
    }
    
    lseco_destroy(handle);
    
    printf(ANSI_COLOR_GREEN "PASS" ANSI_COLOR_RESET "\n");
}

int main() {
    printf("\n");
    printf("==============================================\n");
    printf("  Lseco Secure Memory Library - Test Suite\n");
    printf("==============================================\n\n");
    
    test_version();
    test_error_strings();
    test_create_destroy();
    test_store_retrieve();
    test_size_limits();
    test_multiple_operations();
    test_binary_data();
    
    printf("\n");
    printf(ANSI_COLOR_GREEN "All tests passed! ✓" ANSI_COLOR_RESET "\n\n");
    
    printf("Security Features Verified:\n");
    printf("  ✓ NULL pointer validation\n");
    printf("  ✓ Size boundary checking\n");
    printf("  ✓ No exit/abort on errors (DoS prevention)\n");
    printf("  ✓ Memory lifecycle management\n");
    printf("  ✓ Binary data integrity\n\n");
    
    return 0;
}
