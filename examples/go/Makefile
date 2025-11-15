.PHONY: build test clean

# Build shared library
build:
	@echo "Building Lseco library..."
	@cd ../.. && make
	@echo "Build complete!"

# Run test
test: build
	@echo "Running Go example..."
	@go run main.go

# Clean
clean:
	@echo "Cleaning build artifacts..."
	@cd ../.. && make clean
