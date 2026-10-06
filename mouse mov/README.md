# Infestation - Kernel Mouse Driver

A high-performance Windows kernel-mode driver that provides advanced mouse control and memory access capabilities. Built for Windows 10/11 x64 and ARM64 architectures.

## Overview

Infestation is a kernel-mode driver that operates at Ring 0 privilege level, enabling direct hardware access and system-level operations. It provides a communication interface for user-mode applications to control mouse input and access process memory.

## Core Features

### Mouse Control
- Real-time mouse movement and button control
- Direct injection into Windows mouse class driver
- Support for left, right, and middle button operations
- Both relative and absolute positioning modes

### Process Memory Access
- Read virtual memory from arbitrary processes
- Write virtual memory to target processes
- Safe memory write operations using MDL (Memory Descriptor List)
- Process enumeration and module base address resolution

### Process Information
- Query process IDs by process name
- Retrieve module base addresses for loaded DLLs
- Enumerate kernel modules
- Support for PEB (Process Environment Block) traversal

## Architecture

### Module Structure

```
driver/
├── communication/
│   ├── dispatch.c    - IRP request handling and device control
│   └── dispatch.h    - Communication structures and definitions
├── memory/
│   ├── memory.c      - Kernel module and process enumeration
│   ├── memory.h      - Memory access declarations
│   ├── process.c     - Virtual memory read/write operations
│   └── process.h     - Process memory interface
├── mouse/
│   ├── mouse.asm     - x64 assembly for mouse callback injection
│   └── mouse.h       - Mouse control functions
├── utils/
│   ├── defs.h        - Type definitions and kernel structures
│   └── message.h     - Debug logging macros
└── main.c            - Driver entry point and initialization
```

## Improvements & Enhancements

### Security Hardening
- **Buffer Validation**: Validates input buffer sizes before processing to prevent buffer overflow vulnerabilities
- **Rate Limiting**: Implements request rate limiting (1000 requests/second) to prevent denial-of-service attacks
- **Null Pointer Checks**: Comprehensive null pointer validation on all device extensions and structures
- **Exception Handling**: Structured exception handling with __try/__finally blocks for resource cleanup

### Code Quality
- **Memory Management**: Proper reference counting with ObDereferenceObject for all process handles
- **Resource Cleanup**: Guaranteed cleanup of MDL allocations and kernel objects using __finally blocks
- **Error Handling**: Detailed NTSTATUS error checking and propagation throughout the codebase
- **Code Organization**: Modular architecture with clear separation of concerns

### Bug Fixes
- **Fixed Comparison Bug**: Corrected logical comparison operator in READ_REQUEST handling (dispatch.c:49)
- **Fixed Syntax Error**: Removed malformed preprocessor directive in message.h
- **Fixed Memory Write**: Corrected write_virtual_memory() to properly copy from kernel to target process
- **Fixed Resource Leaks**: Added missing ObDereferenceObject calls in memory access functions

### Performance Optimizations
- **Efficient Process Lookup**: Direct linked-list traversal using hardcoded EPROCESS offsets
- **Minimal Overhead**: Inline functions for mouse operations to reduce call overhead
- **Direct I/O**: Uses METHOD_BUFFERED for efficient kernel-user communication

## Building the Driver

### Requirements
- Visual Studio 2022 (v143 or later)
- Windows Driver Kit (WDK)
- Driver certificate or Windows Test Mode enabled
- Platform Toolset: v143

### Build Configuration
- Debug|x64
- Debug|ARM64
- Release|x64
- Release|ARM64

### Compilation
```bash
# Open in Visual Studio 2022
msbuild km-mouse.sln /p:Configuration=Release /p:Platform=x64
```

## Installation

### Manual Mapping
1. Compile the driver to obtain infestation.sys
2. Use a kernel mapper (kdmapper, lenovo) to load the driver
3. Disable Security Boot if required (see examples/security_check.png)

### Service Installation (Requires Signed Driver)
```bash
# Create service
sc create infestation binPath= "C:\path\to\infestation.sys" type= kernel

# Load driver
sc start infestation

# Stop driver
sc stop infestation

# Delete service
sc delete infestation
```

## Usage

### User-Mode Communication
Applications communicate with the driver via DeviceIoControl:

```c
// Get device handle
HANDLE hDevice = CreateFileA("\\\\.\\infestation", 
    GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);

// Move mouse
KMOUSE_REQUEST req = {500, 300, MOUSE_LEFT_BUTTON_DOWN};
DeviceIoControl(hDevice, MOUSE_REQUEST, &req, sizeof(req), NULL, 0, NULL, NULL);

// Read process memory
KERNEL_READ_REQUEST read_req = {
    .src_pid = target_pid,
    .src_address = (PVOID)0x7FFF0000,
    .p_buffer = &buffer,
    .size = 256
};
DeviceIoControl(hDevice, READ_REQUEST, &read_req, sizeof(read_req), NULL, 0, NULL, NULL);
```

## Technical Details

### Device Control Codes
- `MOUSE_REQUEST (0x666)`: Move mouse or click buttons
- `PROCESSID_REQUEST (0x555)`: Get process ID by name
- `MODULEBASE_REQUEST (0x777)`: Get module base address
- `READ_REQUEST (0x888)`: Read process memory
- `WRITE_REQUEST (0x999)`: Write process memory

### EPROCESS Offsets (Windows 10 22H2)
```
ActiveProcessLinks: 0x448
UniqueProcessId:    0x2e8
ImageFileName:      0x5a8
```

Note: These offsets are specific to Windows 10 Build 22H2 and may vary on other versions.

## Platform Support

| Platform | x64 | ARM64 |
|----------|-----|-------|
| Windows 10 | ✓ | ✓ |
| Windows 11 | ✓ | ✓ |

## Security Considerations

This driver operates at Ring 0 privilege level with unrestricted memory access. It should only be used for:
- Kernel research and development
- Driver testing and validation
- Authorized security testing
- Educational purposes

Misuse for unauthorized memory access or control is illegal and unethical.

## Disclaimer

This project is provided for educational and authorized security research purposes only. Users are responsible for complying with all applicable laws and regulations regarding kernel-mode driver development and usage. The authors assume no liability for unauthorized or illegal use.

## License

GNU General Public License v2 (GPL v2) - See LICENSE file for details

## References

- [Vergilius Project - EPROCESS Offsets](https://www.vergiliusproject.com/kernels/x64/Windows%202010%20%7C%202016)
- [Windows Driver Kit Documentation](https://docs.microsoft.com/en-us/windows-hardware/drivers/)

