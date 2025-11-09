# ITSP Simulator

## Overview

ITSP (Inter-Track Simulcast Protocol) Simulator is a C-based network application that simulates communication between host and remote betting systems for pari-mutuel wagering, primarily focused on horse racing data exchange.

**Author:** F. Baccari  
**Created:** 2017  
**Language:** C  
**Build System:** GNU Make (Cygwin)

## Purpose

This simulator enables testing and development of ITSP protocol implementations without requiring live production systems. It facilitates:

- Inter-track betting data synchronization
- Real-time pool exchange between multiple tracks
- Race lifecycle management
- Betting transaction processing
- Payoff calculations and distribution

## Architecture

### Client-Server Model

```
┌─────────────┐         ITSP Protocol        ┌─────────────┐
│    HOST     │◄──────────────────────────────►│   REMOTE    │
│ (Organizer) │    TCP/IP Socket (Port 4000)  │ (Collector) │
└─────────────┘                                └─────────────┘
```

### Core Components

#### 1. Connection Management
- **`itsp_host.c/h`** - Host server implementation, accepts remote connections
- **`itsp_remote.c/h`** - Remote client implementation, connects to host
- **`itsp_cnx.c/h`** - Common connection handling, socket I/O, frame buffering

**Connection States:**
```
OFF → PHYSICAL → CONFIGURED → SYNCHRONIZED → LOGICAL
```

#### 2. Frame Processing
- **`itsp_frame.c/h`** - Frame structure and basic operations
- **`itsp_frame_maker.c/h`** - Constructs outgoing ITSP frames
- **`itsp_frame_analyser.c/h`** - Parses and validates incoming frames
- **`itsp_header.c/h`** - Header encoding/decoding

**Frame Structure:**
```
┌─────────────────────────────────────┬──────────────┬──────────┐
│  Header (31 bytes)                  │  Data (var)  │ Checksum │
│  [Type|Source|Seq|Event|Race|Pool]  │              │ (5 bytes)│
└─────────────────────────────────────┴──────────────┴──────────┘
      STX (0x02)                                          ETX (0x03)
```

#### 3. Protocol Management
- **`itsp_scheduler.c/h`** - Action sequencing and state management
- **`itsp_pilot.c/h`** - Event handling and connection lifecycle
- **`itsp_translator.c/h`** - Protocol translation layer

#### 4. Data Handlers
Specialized handlers for each ITSP data type:

| Handler | Data Type | Description |
|---------|-----------|-------------|
| `data_link.c/h` | Link | Protocol version negotiation |
| `data_config.c/h` | Config | Track and pool configuration |
| `data_race_status.c/h` | Race Status | Race state, runners, scratches |
| `data_pools.c/h` | Pools | Betting pool amounts |
| `data_scan.c/h` | Scan | Betting combination scans |
| `data_total.c/h` | Totals | Pool totals (live/net) |
| `data_payoffs.c/h` | Payoffs | Final payoff prices |
| `data_results.c/h` | Results | Race finish order |
| `data_alert.c/h` | Alert | System alerts/errors |
| `data_will_pay.c/h` | Will Pay | Estimated payouts |
| `data_file.c/h` | File Transfer | File exchange |

#### 5. Session Management
- **`s3k_session.c/h`** - Session data structures and lifecycle
- **`s3k_commun.c/h`** - S3K protocol communication
- **`s3k_structs.h`** - S3K data structures (tracks, races, pools, bets)

#### 6. Utility Libraries
Custom data structures and helpers:
- **`i_fifo.c/h`** - FIFO queue implementation
- **`i_fast_map.c/h`** - Fast key-value map
- **`i_sorted_set.c/h`** - Sorted set container
- **`i_sorted_vect.c/h`** - Sorted vector container
- **`i_string.c/h`** - String manipulation utilities
- **`i_file.c/h`** - File I/O helpers
- **`i_thread.c/h`** - Threading primitives
- **`i_tools.c/h`** - General utilities
- **`memory_trace.c/h`** - Memory debugging

#### 7. Common Functions
- **`itsp_common.c/h`** - Protocol utilities (date/time, amounts, checksums, combinations)

## ITSP Protocol

### Data Types

| Code | Type | Description |
|------|------|-------------|
| `L` | Link | Protocol handshake and version |
| `C` | Config | Track/pool configuration |
| `S` | Race Status | Race state and runner status |
| `P` | Pools | Betting pool data |
| `X` | Scan | Combination scan requests |
| `T` | Totals | Pool total amounts |
| `$` | Payoffs | Final prices and dividends |
| `R` | Results | Race finish positions |
| `A` | Alert | System alerts |
| `W` | Will Pay | Estimated payouts |
| `F` | File Transfer | File exchange |

### Message Types

| Code | Type | Usage |
|------|------|-------|
| `P` | Pending | Notification of upcoming data |
| `R` | Request | Data request |
| `D` | Data | Data transmission |
| `A` | Acknowledge | Confirmation/response |

### Reason Codes

| Code | Reason | Description |
|------|--------|-------------|
| `0` | Fault | Generic error |
| `1` | OK | Success |
| `b` | Begin | Sequence start |
| `e` | End | Sequence end |
| `f` | Final | Final data |
| `H` | Invalid Header | Header validation failed |
| `J` | Invalid Data | Data validation failed |
| `D` | Data Checksum | Checksum error |
| `R` | Invalid Race | Race not found |
| `P` | Invalid Pool | Pool not found |
| `C` | Race Closed | Race is closed |
| `X` | Terminate | Sequence termination |

### Connection Types

- **Chrono** - Timing/synchronization only
- **Pool** - Betting pool data only
- **Chrono-Pool** - Combined timing and pool data

## Pool Types

Supported betting pool types:

| Code | Name | Dimensions | Description |
|------|------|------------|-------------|
| WIN | Win | 1 | Pick winner |
| PLC | Place | 1 | Pick top 2-3 finishers |
| SHW | Show | 1 | Pick top 3 finishers |
| QU | Quinella | 2 | Pick 2 in any order |
| EX | Exacta | 2 | Pick 2 in exact order |
| TRI | Trifecta | 3 | Pick 3 in exact order |
| SUP | Superfecta | 4 | Pick 4 in exact order |
| DD | Daily Double | 2 | Winners of 2 consecutive races |
| PK3 | Pick 3 | 3 | Winners of 3 consecutive races |

### Scan Modes

| Code | Mode | Description |
|------|------|-------------|
| `P` | Pool | Standard pool scan |
| `E` | Early | Early betting combinations |
| `K` | Combination Early | Complex early combinations |
| `Q` | Quinella | Quinella-style scan |
| `A` | Alternate Runner | Alternate runner mode |
| `L` | Late | Late betting combinations |
| `S` | Superfecta | Superfecta scan |
| `X` | Exact N | Exact N-way combinations |
| `C` | Combination Late | Complex late combinations |

## Building

### Prerequisites
- GCC compiler
- Make
- POSIX threads library
- Cygwin (for Windows) or Linux environment

### Compilation

```bash
cd Debug
make clean
make all
```

This produces `itsp_simulator.exe`

### Build Configuration

The build system uses:
- `makefile` - Main build file
- `sources.mk` - Source file definitions
- `objects.mk` - Object file rules
- `source/subdir.mk` - Source subdirectory rules

## Usage

### Running the Simulator

```c
// Initialize host server
p_s_itsp_host host = itsp_host_init("NDT", itsp_chrono_pool, "log_host");
start_host_server(host);

// Initialize remote client
p_s_itsp_remote remote = itsp_remote_init("OJ1", "LOL", itsp_chrono_pool, "log_remote");
start_remote_client(remote);

// ... operations ...

// Shutdown
stop_remote_client(remote);
stop_host_server(host);
```

### Loading Session from Log

```c
// Load historical session data
load_session_from_log("log/ITSP_resitsp6201_hes_R1");
```

## Configuration

### Constants

```c
#define SOCK_BUFF_SIZE 4095            // Socket buffer size
#define CNX_SELECT_TIMEOUT_S 2         // Select timeout (seconds)
#define CNX_MAX_TIMEOUT_S 15           // Connection timeout
#define MAX_SRV_CXN 1023               // Max server connections
#define ITSP_NBMAX_RACES 30            // Max races per event
#define ITSP_NBMAX_RUNNERS 30          // Max runners per race
#define ITSP_NBMAX_POOLS 30            // Max pools per race
```

### Default Settings

- **Port:** 4000
- **Address:** INADDR_ANY (0.0.0.0)
- **Protocol Family:** AF_INET (IPv4)
- **Thread Model:** POSIX threads

## Test Data

The `log/` directory contains historical session logs for testing:

- **Soccer races** - Various soccer-based betting logs
- **Track logs** - Real track data (PMC, GRX, etc.)
- **Amtote logs** - Amtote system integration logs
- **Replicated sessions** - Host/remote replication tests

Example log output shows:
- 24,434 frames loaded
- 8 races with 15-16 runners each
- 5 pools per race (WIN, PLACE, SHOW, QUINELLA, EXACTA)
- Complete race lifecycle from open to official results

## Key Features

### 1. Multi-Threading
- Concurrent connection handling
- Thread-safe data structures
- Mutex-protected state management

### 2. Event-Driven Architecture
- Event types: connect, disconnect, receive, send, timeout, error
- State machine for connection lifecycle
- Action sequences for protocol flow

### 3. Data Validation
- Header checksum validation
- Data checksum verification
- Protocol compliance checking
- Race/pool/runner validation

### 4. Pool Management
- Gross vs. net pool calculations
- Commission rate handling (multiple tiers)
- Rounding modes (up, down, true)
- Minimum bet enforcement
- Break amount calculations

### 5. Combination Handling
- Flexible combination parsing
- Boxing and keying support
- Wheel combinations
- Range specifications (e.g., "1-5,7,9-12")

### 6. Amount Precision
- Double-precision floating point
- Configurable precision (typically 2 decimal places)
- Currency formatting
- Tolerance handling (0.00001)

## Error Handling

### Return Codes

```c
typedef enum ret_code {
    err_pools       = -12,  // Pool error
    err_abo_pool    = -11,  // Subscription pool error
    err_abo_race    = -10,  // Subscription race error
    err_abo         = -9,   // Subscription error
    err_protocol    = -8,   // Protocol error
    err_config      = -7,   // Configuration error
    err_null_param  = -6,   // NULL parameter
    err_type        = -5,   // Type error
    err_att         = -4,   // Attribute error
    err_grep        = -3,   // Track error
    err_race        = -2,   // Race error
    err_pool        = -1,   // Pool error
    err_default     = 0,    // Default/unknown error
    ret_ok          = 1     // Success
} ret_code;
```

## Development

### File Organization

```
itsp_simulator/
├── include/          # Header files
│   ├── itsp_*.h     # ITSP protocol headers
│   ├── data_*.h     # Data type handlers
│   ├── i_*.h        # Utility library headers
│   └── s3k_*.h      # S3K session headers
├── source/           # Source files
│   ├── itsp_*.c     # ITSP protocol implementation
│   ├── data_*.c     # Data type handlers
│   ├── i_*.c        # Utility library implementation
│   ├── s3k_*.c      # S3K session implementation
│   └── main.c       # Entry point
├── Debug/            # Build directory
│   ├── makefile     # Build configuration
│   └── source/      # Compiled objects
├── log/              # Test logs and session data
└── README.md         # This file
```

### Debugging

- Memory tracing available via `memory_trace.c/h`
- Logging to stdout or file
- Frame-level logging for protocol debugging
- Sequence logging for state tracking

### Adding New Pool Types

1. Add pool configuration to `data_config.c`
2. Implement scan mode in `data_scan.c`
3. Add payoff calculation in `data_payoffs.c`
4. Update `s3k_structs.h` with pool type definition

## Technical Specifications

### Performance
- **Frame processing:** Real-time
- **Connection capacity:** Up to 1023 concurrent connections
- **Throughput:** Limited by socket buffer (4KB)
- **Latency:** Sub-second response times

### Scalability
- Multi-race events (up to 30 races)
- Multiple pool types per race (up to 30)
- Large runner fields (up to 30 runners)
- Complex combinations (arbitrary complexity)

### Reliability
- Checksum validation on all frames
- Timeout detection and recovery
- Connection state monitoring
- Error propagation and handling

## Use Cases

1. **Protocol Testing** - Validate ITSP implementations
2. **Integration Testing** - Test inter-track connections
3. **Development** - Protocol development and debugging
4. **Training** - Learn pari-mutuel betting systems
5. **Analysis** - Replay and analyze historical sessions
6. **Regression Testing** - Verify protocol changes
7. **Load Testing** - Stress test with multiple connections

## Limitations

- Windows/Cygwin specific build (portable with modifications)
- Single-threaded frame processing per connection
- Fixed buffer sizes
- No persistent storage (memory-only)
- Limited to ITSP protocol version 5.18

## Future Enhancements

- [ ] Linux native build support
- [ ] Configuration file support
- [ ] Web interface for monitoring
- [ ] Database integration for persistence
- [ ] Enhanced logging and metrics
- [ ] Protocol version negotiation
- [ ] SSL/TLS support
- [ ] IPv6 support

## License

Not specified in source files.

## Contact

**Author:** F. Baccari  
**Year:** 2017

## References

- ITSP Protocol Specification (implied version 5.18)
- Pari-mutuel wagering systems
- Inter-track betting standards

---

*Last Updated: November 9, 2025*
