# Node.js FFI Example for Lseco

This example demonstrates how to use Lseco from Node.js using `ffi-napi`.

## Prerequisites

```bash
npm install ffi-napi ref-napi
```

**Note:** `ffi-napi` requires native compilation. If you encounter build errors with Node.js 20+, consider:
- Using Node.js 18 LTS: `nvm use 18`
- Or use alternative: `koffi` package (faster, no compilation needed)

### Alternative: Using Koffi (Recommended)

```bash
npm install koffi
```

Koffi is a modern FFI library that doesn't require compilation and works with Node.js 20+.

## Installation

```bash
# From examples/node directory
npm install
```

## Usage

```bash
# Make sure the library is built
cd ../..
make

# Run the example
cd examples/node
node index.js
```

## Features Demonstrated

### Success Cases
- ✅ Creating secure storage
- ✅ Storing and retrieving passwords
- ✅ Storing and retrieving API keys
- ✅ Proper cleanup

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
- Error handling
- Buffer conversion
- TypeScript-like interface

## Example Output

```
==================================================
  Lseco Node.js Example - Comprehensive Test
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

- Always call `destroy()` to free C-allocated memory
- Node.js GC cannot free C memory automatically
- For production, add proper error handling and logging
- Thread safety requires external synchronization

## Common Issues

### `Error: Dynamic Linking Error`
Make sure the library path is correct:
```javascript
// macOS
const libPath = path.join(__dirname, '../../liblseco.dylib');
// Linux
const libPath = path.join(__dirname, '../../liblseco.so');
// Windows
const libPath = path.join(__dirname, '../../lseco.dll');
```

### `Module did not self-register`
Rebuild `ffi-napi` for your Node.js version:
```bash
npm rebuild ffi-napi
```
