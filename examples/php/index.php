<?php

/**
 * Lseco PHP FFI Example
 * 
 * Demonstrates secure memory storage using Lseco library via PHP FFI.
 */

// Determine library path based on platform
$libPath = null;
if (PHP_OS_FAMILY === 'Darwin') {
    $libPath = __DIR__ . '/../../liblseco.dylib';
} elseif (PHP_OS_FAMILY === 'Linux') {
    $libPath = __DIR__ . '/../../liblseco.so';
} elseif (PHP_OS_FAMILY === 'Windows') {
    $libPath = __DIR__ . '/../../lseco.dll';
} else {
    die("Unsupported platform: " . PHP_OS_FAMILY . "\n");
}

if (!file_exists($libPath)) {
    die("Library not found: $libPath\nPlease build the library first: make\n");
}

// Load library using FFI
$ffi = FFI::cdef("
    typedef void* lseco_handle_t;
    
    lseco_handle_t lseco_create(size_t size);
    int lseco_store(lseco_handle_t handle, const void* data, size_t length);
    int lseco_retrieve(lseco_handle_t handle, void* buffer, size_t length);
    size_t lseco_get_size(lseco_handle_t handle);
    void lseco_destroy(lseco_handle_t handle);
    const char* lseco_error_string(int error_code);
    const char* lseco_version(void);
", $libPath);

// Error codes
const LSECO_SUCCESS = 0;
const LSECO_ERR_NULL_PTR = -1;
const LSECO_ERR_ALLOC_FAILED = -2;
const LSECO_ERR_LOCK_FAILED = -3;
const LSECO_ERR_PROTECT_FAILED = -4;
const LSECO_ERR_INVALID_SIZE = -5;

/**
 * SecureStorage class - wraps Lseco FFI with PHP-friendly interface
 */
class SecureStorage {
    private $ffi;
    private $handle;
    private int $size;

    public function __construct($ffi, int $size) {
        if ($size <= 0) {
            throw new InvalidArgumentException('Size must be greater than 0');
        }

        $this->ffi = $ffi;
        $this->handle = $ffi->lseco_create($size);

        if ($this->handle === null || FFI::isNull($this->handle)) {
            throw new RuntimeException('Failed to create secure storage');
        }

        $this->size = $size;
    }

    /**
     * Store data in secure memory
     */
    public function store(string $data): void {
        if ($this->handle === null) {
            throw new RuntimeException('Storage already destroyed');
        }

        if (strlen($data) === 0) {
            throw new InvalidArgumentException('Data cannot be empty');
        }

        if (strlen($data) > $this->size) {
            throw new InvalidArgumentException(
                sprintf('Data size %d exceeds storage size %d', strlen($data), $this->size)
            );
        }

        $result = $this->ffi->lseco_store($this->handle, $data, strlen($data));

        if ($result !== LSECO_SUCCESS) {
            $errorMsg = $this->ffi->lseco_error_string($result);
            throw new RuntimeException('Store failed: ' . FFI::string($errorMsg));
        }
    }

    /**
     * Retrieve data from secure memory
     */
    public function retrieve(int $length): string {
        if ($this->handle === null) {
            throw new RuntimeException('Storage already destroyed');
        }

        if ($length <= 0 || $length > $this->size) {
            throw new InvalidArgumentException(
                sprintf('Invalid length %d (max: %d)', $length, $this->size)
            );
        }

        $buffer = $this->ffi->new("char[$length]");
        $result = $this->ffi->lseco_retrieve($this->handle, $buffer, $length);

        if ($result !== LSECO_SUCCESS) {
            $errorMsg = $this->ffi->lseco_error_string($result);
            throw new RuntimeException('Retrieve failed: ' . FFI::string($errorMsg));
        }

        return FFI::string($buffer, $length);
    }

    /**
     * Get allocated storage size
     */
    public function getSize(): int {
        if ($this->handle === null) {
            return 0;
        }
        return $this->ffi->lseco_get_size($this->handle);
    }

    /**
     * Destroy secure storage and free memory
     */
    public function destroy(): void {
        if ($this->handle !== null) {
            $this->ffi->lseco_destroy($this->handle);
            $this->handle = null;
        }
    }

    /**
     * Destructor - ensure cleanup
     */
    public function __destruct() {
        $this->destroy();
    }
}

// ============================================================================
// Demonstration Functions
// ============================================================================

function demonstrateSuccessCases($ffi): void {
    echo "=== Success Cases ===\n\n";

    $storage = new SecureStorage($ffi, 256);

    try {
        // Store password
        $password = 'SuperSecret123!';
        echo "✓ Storing password: $password\n";
        $storage->store($password);

        // Retrieve password
        $retrieved = $storage->retrieve(strlen($password) + 1);
        $retrieved = rtrim($retrieved, "\0");
        echo "✓ Retrieved password: $retrieved\n";

        // Store API key
        $apiKey = 'sk-1234567890abcdef';
        echo "✓ Storing API key: $apiKey\n";
        $storage->store($apiKey);

        // Retrieve API key
        $retrievedKey = $storage->retrieve(strlen($apiKey) + 1);
        $retrievedKey = rtrim($retrievedKey, "\0");
        echo "✓ Retrieved API key: $retrievedKey\n";
        echo "\n";
    } finally {
        $storage->destroy();
    }
}

function demonstrateFailureCases($ffi): void {
    echo "=== Failure Cases (Error Handling) ===\n\n";

    // Case 1: Create with invalid size
    echo "1. Creating storage with size 0:\n";
    try {
        new SecureStorage($ffi, 0);
        echo "   ✗ Should have failed!\n";
    } catch (Exception $e) {
        echo "   ✗ Expected error: {$e->getMessage()}\n";
    }
    echo "\n";

    // Case 2: Store empty data
    echo "2. Storing empty data:\n";
    $storage = new SecureStorage($ffi, 256);
    try {
        $storage->store('');
        echo "   ✗ Should have failed!\n";
    } catch (Exception $e) {
        echo "   ✗ Expected error: {$e->getMessage()}\n";
    } finally {
        $storage->destroy();
    }
    echo "\n";

    // Case 3: Store data larger than size
    echo "3. Storing data larger than storage size:\n";
    $smallStorage = new SecureStorage($ffi, 16);
    try {
        $largeData = 'This is a very long string that exceeds 16 bytes';
        $smallStorage->store($largeData);
        echo "   ✗ Should have failed!\n";
    } catch (Exception $e) {
        echo "   ✗ Expected error: {$e->getMessage()}\n";
    } finally {
        $smallStorage->destroy();
    }
    echo "\n";

    // Case 4: Retrieve with invalid length
    echo "4. Retrieving with length 0:\n";
    $storage2 = new SecureStorage($ffi, 256);
    try {
        $storage2->retrieve(0);
        echo "   ✗ Should have failed!\n";
    } catch (Exception $e) {
        echo "   ✗ Expected error: {$e->getMessage()}\n";
    } finally {
        $storage2->destroy();
    }
    echo "\n";

    // Case 5: Retrieve more than allocated
    echo "5. Retrieving more than storage size:\n";
    $storage3 = new SecureStorage($ffi, 256);
    try {
        $storage3->retrieve(512);
        echo "   ✗ Should have failed!\n";
    } catch (Exception $e) {
        echo "   ✗ Expected error: {$e->getMessage()}\n";
    } finally {
        $storage3->destroy();
    }
    echo "\n";

    // Case 6: Multiple destroy calls
    echo "6. Using storage after destroy:\n";
    $storage4 = new SecureStorage($ffi, 64);
    $storage4->destroy();
    echo "   ✓ Destroy is safe to call multiple times\n";
    $storage4->destroy(); // Safe to call again
    echo "\n";
}

function demonstrateEdgeCases($ffi): void {
    echo "=== Edge Cases ===\n\n";

    // Case 1: Minimum valid size
    echo "1. Creating storage with size 1:\n";
    try {
        $storage = new SecureStorage($ffi, 1);
        echo "   ✓ Success! Can create 1-byte storage\n";

        $data = "\x42";
        $storage->store($data);
        echo "   ✓ Stored 1 byte\n";

        $retrieved = $storage->retrieve(1);
        echo sprintf("   ✓ Retrieved: 0x%02X\n", ord($retrieved[0]));

        $storage->destroy();
    } catch (Exception $e) {
        echo "   ✗ Error: {$e->getMessage()}\n";
    }
    echo "\n";

    // Case 2: Large size allocation
    echo "2. Creating large storage (1 MB):\n";
    try {
        $largeStorage = new SecureStorage($ffi, 1024 * 1024);
        echo "   ✓ Success! Allocated 1 MB\n";
        $largeStorage->destroy();
    } catch (Exception $e) {
        echo "   ✗ Error: {$e->getMessage()}\n";
    }
    echo "\n";

    // Case 3: Binary data with null bytes
    echo "3. Storing binary data with null bytes:\n";
    $storage = new SecureStorage($ffi, 256);
    try {
        $binaryData = "\x00\x01\x02\x00\xFF\xFE\x00";
        $storage->store($binaryData);
        echo "   ✓ Stored binary data with nulls\n";

        $retrieved = $storage->retrieve(strlen($binaryData));
        $bytes = array_map('ord', str_split($retrieved));
        echo "   ✓ Retrieved: [" . implode(', ', $bytes) . "]\n";
    } catch (Exception $e) {
        echo "   ✗ Error: {$e->getMessage()}\n";
    } finally {
        $storage->destroy();
    }
    echo "\n";

    // Case 4: Overwriting data
    echo "4. Overwriting data multiple times:\n";
    $storage2 = new SecureStorage($ffi, 64);
    try {
        for ($i = 1; $i <= 3; $i++) {
            $data = "Data version $i";
            $storage2->store($data);
            $retrieved = $storage2->retrieve(strlen($data) + 1);
            $retrieved = rtrim($retrieved, "\0");
            echo "   ✓ Write $i: $retrieved\n";
        }
    } catch (Exception $e) {
        echo "   ✗ Error: {$e->getMessage()}\n";
    } finally {
        $storage2->destroy();
    }
    echo "\n";
}

// ============================================================================
// Main
// ============================================================================

function main(): void {
    global $ffi;

    echo "\n";
    echo "==================================================\n";
    echo "  Lseco PHP Example - Comprehensive Test\n";
    echo "==================================================\n";
    echo "\n";

    // Print version
    $version = $ffi->lseco_version();
    echo "Library version: $version\n";
    echo "\n";

    // Demonstrate different scenarios
    demonstrateSuccessCases($ffi);
    demonstrateFailureCases($ffi);
    demonstrateEdgeCases($ffi);

    echo "==================================================\n";
    echo "✓ All demonstrations completed successfully!\n";
    echo "==================================================\n";
    echo "\n";
}

// Run
try {
    main();
} catch (Exception $e) {
    fwrite(STDERR, "Fatal error: {$e->getMessage()}\n");
    exit(1);
}
