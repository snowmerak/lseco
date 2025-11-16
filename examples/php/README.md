# PHP FFI Example for Lseco

This example demonstrates how to use Lseco from PHP using FFI (Foreign Function Interface).

## Prerequisites

- **PHP 7.4 or later** (FFI support)
- **FFI extension enabled** in php.ini

### Check FFI support

```bash
php -m | grep FFI
```

If not enabled, add to php.ini:
```ini
extension=ffi
ffi.enable=true
```

## Usage

```bash
# Make sure the library is built
cd ../..
make

# Run the example
cd examples/php
php index.php
```

## Features Demonstrated

### Success Cases
- ✅ Creating secure storage
- ✅ Storing and retrieving passwords
- ✅ Storing and retrieving API keys
- ✅ Proper cleanup with try-finally

### Failure Cases (Error Handling)
- ✅ Creating storage with invalid size (0)
- ✅ Storing empty data
- ✅ Storing data larger than allocated size
- ✅ Retrieving with invalid length
- ✅ Safe multiple destroy calls

### Edge Cases
- ✅ Minimum valid size (1 byte)
- ✅ Large allocations (1 MB)
- ✅ Binary data with null bytes
- ✅ Overwriting data multiple times

## API Wrapper

The example includes a `SecureStorage` class that wraps the C API with:
- Automatic memory management
- Exception-based error handling
- String conversion utilities
- Object-oriented interface

## Example Output

```
==================================================
  Lseco PHP Example - Comprehensive Test
==================================================

Library version: 1.0.0

=== Success Cases ===

✓ Storing password: SuperSecret123!
✓ Retrieved password: SuperSecret123!
✓ Storing API key: sk-1234567890abcdef
✓ Retrieved API key: sk-1234567890abcdef

=== Failure Cases (Error Handling) ===

1. Creating storage with size 0:
   ✗ Expected error: Failed to create secure storage

2. Storing empty data:
   ✗ Expected error: Data cannot be empty

3. Storing data larger than storage size:
   ✗ Expected error: Data size 48 exceeds storage size 16

[... more tests ...]
```

## Notes

- Always call `destroy()` or use try-finally to free C-allocated memory
- PHP GC cannot free C memory automatically
- For production, add proper error handling and logging
- Thread safety requires external synchronization

## Common Issues

### `FFI\Exception: Failed loading`
Make sure the library path is correct for your platform:
```php
// macOS
$libPath = __DIR__ . '/../../liblseco.dylib';
// Linux
$libPath = __DIR__ . '/../../liblseco.so';
// Windows
$libPath = __DIR__ . '/../../lseco.dll';
```

### `Call to undefined function FFI::cdef()`
FFI extension is not enabled. Check php.ini:
```bash
php --ini
```

Add to php.ini:
```ini
extension=ffi
ffi.enable=true
```

### Performance Tips

For production use, consider preloading the FFI definition:
```php
// preload.php
opcache_compile_file(__DIR__ . '/SecureStorage.php');
```

## Platform-Specific Notes

### macOS
May need to set library path:
```bash
export DYLD_LIBRARY_PATH=/path/to/lseco:$DYLD_LIBRARY_PATH
php index.php
```

### Linux
May need to set library path:
```bash
export LD_LIBRARY_PATH=/path/to/lseco:$LD_LIBRARY_PATH
php index.php
```

### Windows
Make sure `lseco.dll` is in the same directory or in system PATH.
