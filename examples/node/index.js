const koffi = require('koffi');
const path = require('path');
const os = require('os');

// Determine library path based on platform
let libPath;
const platform = os.platform();
if (platform === 'darwin') {
    libPath = path.join(__dirname, '../../liblseco.dylib');
} else if (platform === 'linux') {
    libPath = path.join(__dirname, '../../liblseco.so');
} else if (platform === 'win32') {
    libPath = path.join(__dirname, '../../lseco.dll');
} else {
    throw new Error(`Unsupported platform: ${platform}`);
}

// Load library
const lib = koffi.load(libPath);

// Define functions
const lseco_create = lib.func('lseco_create', 'void*', ['size_t']);
const lseco_store = lib.func('lseco_store', 'int', ['void*', 'void*', 'size_t']);
const lseco_retrieve = lib.func('lseco_retrieve', 'int', ['void*', 'void*', 'size_t']);
const lseco_get_size = lib.func('lseco_get_size', 'size_t', ['void*']);
const lseco_destroy = lib.func('lseco_destroy', 'void', ['void*']);
const lseco_error_string = lib.func('lseco_error_string', 'string', ['int']);
const lseco_version = lib.func('lseco_version', 'string', []);

// Error codes
const LSECO_SUCCESS = 0;
const LSECO_ERR_NULL_PTR = -1;
const LSECO_ERR_ALLOC_FAILED = -2;
const LSECO_ERR_LOCK_FAILED = -3;
const LSECO_ERR_PROTECT_FAILED = -4;
const LSECO_ERR_INVALID_SIZE = -5;

/**
 * SecureStorage class - wraps Lseco FFI with Node.js-friendly interface
 */
class SecureStorage {
    constructor(size) {
        if (size <= 0) {
            throw new Error('Size must be greater than 0');
        }

        this.handle = lseco_create(size);
        if (this.handle === null || koffi.address(this.handle) === 0n) {
            throw new Error('Failed to create secure storage');
        }

        this.size = size;
    }

    /**
     * Store data in secure memory
     * @param {Buffer|string} data - Data to store
     */
    store(data) {
        if (this.handle === null || koffi.address(this.handle) === 0n) {
            throw new Error('Storage already destroyed');
        }

        // Convert string to Buffer if needed
        const buffer = Buffer.isBuffer(data) ? data : Buffer.from(data, 'utf8');

        if (buffer.length === 0) {
            throw new Error('Data cannot be empty');
        }

        if (buffer.length > this.size) {
            throw new Error(`Data size ${buffer.length} exceeds storage size ${this.size}`);
        }

        const result = lseco_store(this.handle, buffer, buffer.length);
        if (result !== LSECO_SUCCESS) {
            const errorMsg = lseco_error_string(result);
            throw new Error(`Store failed: ${errorMsg}`);
        }
    }

    /**
     * Retrieve data from secure memory
     * @param {number} length - Number of bytes to retrieve
     * @returns {Buffer} Retrieved data
     */
    retrieve(length) {
        if (this.handle === null || koffi.address(this.handle) === 0n) {
            throw new Error('Storage already destroyed');
        }

        if (length <= 0 || length > this.size) {
            throw new Error(`Invalid length ${length} (max: ${this.size})`);
        }

        const buffer = Buffer.alloc(length);
        const result = lseco_retrieve(this.handle, buffer, length);

        if (result !== LSECO_SUCCESS) {
            const errorMsg = lseco_error_string(result);
            throw new Error(`Retrieve failed: ${errorMsg}`);
        }

        return buffer;
    }

    /**
     * Get allocated storage size
     * @returns {number} Size in bytes
     */
    getSize() {
        if (this.handle === null || koffi.address(this.handle) === 0n) {
            return 0;
        }
        return lseco_get_size(this.handle);
    }

    /**
     * Destroy secure storage and free memory
     */
    destroy() {
        if (this.handle !== null && koffi.address(this.handle) !== 0n) {
            lseco_destroy(this.handle);
            this.handle = null;
        }
    }
}

// ============================================================================
// Demonstration Functions
// ============================================================================

function demonstrateSuccessCases() {
    console.log('=== Success Cases ===');
    console.log();

    const storage = new SecureStorage(256);

    try {
        // Store password
        const password = 'SuperSecret123!';
        console.log(`✓ Storing password: ${password}`);
        storage.store(password);

        // Retrieve password
        const retrieved = storage.retrieve(password.length + 1);
        console.log(`✓ Retrieved password: ${retrieved.toString('utf8').replace(/\0/g, '')}`);

        // Store API key
        const apiKey = 'sk-1234567890abcdef';
        console.log(`✓ Storing API key: ${apiKey}`);
        storage.store(apiKey);

        // Retrieve API key
        const retrievedKey = storage.retrieve(apiKey.length + 1);
        console.log(`✓ Retrieved API key: ${retrievedKey.toString('utf8').replace(/\0/g, '')}`);
        console.log();
    } finally {
        storage.destroy();
    }
}

function demonstrateFailureCases() {
    console.log('=== Failure Cases (Error Handling) ===');
    console.log();

    // Case 1: Create with invalid size
    console.log('1. Creating storage with size 0:');
    try {
        new SecureStorage(0);
        console.log('   ✗ Should have failed!');
    } catch (err) {
        console.log(`   ✗ Expected error: ${err.message}`);
    }
    console.log();

    // Case 2: Store empty data
    console.log('2. Storing empty data:');
    const storage = new SecureStorage(256);
    try {
        storage.store('');
        console.log('   ✗ Should have failed!');
    } catch (err) {
        console.log(`   ✗ Expected error: ${err.message}`);
    } finally {
        storage.destroy();
    }
    console.log();

    // Case 3: Store data larger than size
    console.log('3. Storing data larger than storage size:');
    const smallStorage = new SecureStorage(16);
    try {
        const largeData = 'This is a very long string that exceeds 16 bytes';
        smallStorage.store(largeData);
        console.log('   ✗ Should have failed!');
    } catch (err) {
        console.log(`   ✗ Expected error: ${err.message}`);
    } finally {
        smallStorage.destroy();
    }
    console.log();

    // Case 4: Retrieve with invalid length
    console.log('4. Retrieving with length 0:');
    const storage2 = new SecureStorage(256);
    try {
        storage2.retrieve(0);
        console.log('   ✗ Should have failed!');
    } catch (err) {
        console.log(`   ✗ Expected error: ${err.message}`);
    } finally {
        storage2.destroy();
    }
    console.log();

    // Case 5: Retrieve more than allocated
    console.log('5. Retrieving more than storage size:');
    const storage3 = new SecureStorage(256);
    try {
        storage3.retrieve(512);
        console.log('   ✗ Should have failed!');
    } catch (err) {
        console.log(`   ✗ Expected error: ${err.message}`);
    } finally {
        storage3.destroy();
    }
    console.log();

    // Case 6: Multiple destroy calls
    console.log('6. Using storage after destroy:');
    const storage4 = new SecureStorage(64);
    storage4.destroy();
    console.log('   ✓ Destroy is safe to call multiple times');
    storage4.destroy(); // Safe to call again
    console.log();
}

function demonstrateEdgeCases() {
    console.log('=== Edge Cases ===');
    console.log();

    // Case 1: Minimum valid size
    console.log('1. Creating storage with size 1:');
    try {
        const storage = new SecureStorage(1);
        console.log('   ✓ Success! Can create 1-byte storage');

        const data = Buffer.from([0x42]);
        storage.store(data);
        console.log('   ✓ Stored 1 byte');

        const retrieved = storage.retrieve(1);
        console.log(`   ✓ Retrieved: 0x${retrieved[0].toString(16).toUpperCase()}`);

        storage.destroy();
    } catch (err) {
        console.log(`   ✗ Error: ${err.message}`);
    }
    console.log();

    // Case 2: Large size allocation
    console.log('2. Creating large storage (1 MB):');
    try {
        const largeStorage = new SecureStorage(1024 * 1024);
        console.log('   ✓ Success! Allocated 1 MB');
        largeStorage.destroy();
    } catch (err) {
        console.log(`   ✗ Error: ${err.message}`);
    }
    console.log();

    // Case 3: Binary data with null bytes
    console.log('3. Storing binary data with null bytes:');
    const storage = new SecureStorage(256);
    try {
        const binaryData = Buffer.from([0x00, 0x01, 0x02, 0x00, 0xFF, 0xFE, 0x00]);
        storage.store(binaryData);
        console.log('   ✓ Stored binary data with nulls');

        const retrieved = storage.retrieve(binaryData.length);
        console.log(`   ✓ Retrieved: [${Array.from(retrieved).join(', ')}]`);
    } catch (err) {
        console.log(`   ✗ Error: ${err.message}`);
    } finally {
        storage.destroy();
    }
    console.log();

    // Case 4: Overwriting data
    console.log('4. Overwriting data multiple times:');
    const storage2 = new SecureStorage(64);
    try {
        for (let i = 1; i <= 3; i++) {
            const data = `Data version ${i}`;
            storage2.store(data);
            const retrieved = storage2.retrieve(data.length + 1);
            console.log(`   ✓ Write ${i}: ${retrieved.toString('utf8').replace(/\0/g, '')}`);
        }
    } catch (err) {
        console.log(`   ✗ Error: ${err.message}`);
    } finally {
        storage2.destroy();
    }
    console.log();
}

// ============================================================================
// Main
// ============================================================================

function main() {
    console.log();
    console.log('==================================================');
    console.log('  Lseco Node.js Example - Comprehensive Test');
    console.log('==================================================');
    console.log();

    // Print version
    const version = lseco_version();
    console.log(`Library version: ${version}`);
    console.log();

    // Demonstrate different scenarios
    demonstrateSuccessCases();
    demonstrateFailureCases();
    demonstrateEdgeCases();

    console.log('==================================================');
    console.log('✓ All demonstrations completed successfully!');
    console.log('==================================================');
    console.log();
}

// Run if executed directly
if (require.main === module) {
    try {
        main();
    } catch (err) {
        console.error('Fatal error:', err.message);
        process.exit(1);
    }
}

// Export for use as module
module.exports = { SecureStorage };
