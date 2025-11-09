# Building ITSP Simulator

## Prerequisites

- GCC compiler
- GNU Make
- POSIX threads library (pthread)
- Math library (libm)

On Ubuntu/Debian:
```bash
sudo apt-get install build-essential
```

## Build Instructions

### Linux

1. Navigate to the project root:
```bash
cd /home/farouk/workspace/itsp_simulator
```

2. Build the project:
```bash
make all
```

3. Clean build artifacts:
```bash
make clean
```

4. Rebuild from scratch:
```bash
make rebuild
```

## Build Output

- **Executable:** `itsp_simulator`
- **Object files:** `source/*.o`

## Running the Simulator

After building, run the simulator:
```bash
./itsp_simulator
```

## Build Configuration

The Makefile includes:
- **Compiler:** GCC
- **Optimization:** -O2
- **Warnings:** -Wall (all warnings enabled)
- **Include path:** `include/`
- **Libraries:** pthread, math

## Troubleshooting

### Missing pthread
If you get pthread-related errors:
```bash
sudo apt-get install libc6-dev
```

### Missing math library
The math library is part of glibc and should be available by default on Linux systems.

## Original Build System

The project originally included a Cygwin/Windows build system in the `Debug/` directory. The root-level `Makefile` is designed for native Linux builds.

## Notes

- The build produces several warnings about array bounds and unused return values, but these don't prevent successful compilation
- The executable is approximately 230KB in size
- Build time is typically under 10 seconds on modern hardware
