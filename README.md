# JBOD Client-Server System

## Overview
This project implements a client-server system for a JBOD (Just a Bunch of Disks) storage system. The client connects to a server and sends operations to be performed on the JBOD, such as reading or writing data blocks. The project is written in C and utilizes various libraries and system calls for network communication and disk operations.

## Project Structure
```
├── jbod-m1.o
├── jbod.h
├── jbod.o
├── Makefile
├── mdadm.c
├── mdadm.h
├── tester.c
├── tester.h
├── util.c
├── util.h
└── traces/
    ├── linear-expected-output
    ├── linear-input
    ├── random-expected-output
    ├── random-input
    ├── simple-expected-output
    └── simple-input
```

## Components

### JBOD Client Library
The client library provides functions for connecting to the JBOD server, sending operation requests, and processing responses.

#### Key Functions

1. **jbod_connect(const char \*ip, uint16_t port)**
   - Creates a socket and connects to the specified server IP and port
   - Sets the global `cli_sd` variable on successful connection
   - Returns `true` on successful connection, `false` otherwise

2. **jbod_disconnect(void)**
   - Closes the connection to the server by closing the `cli_sd` socket
   - Resets `cli_sd` to -1

3. **jbod_client_operation(uint32_t op, uint8_t \*block)**
   - Main operation function that sends JBOD operations to the server
   - Takes an operation code and optionally a data block
   - Returns the response code from the server

### Networking Functions

1. **send_packet(int sd, uint32_t op, uint8_t \*block)**
   - Packages the operation code and data into a packet
   - Packet format:
     - 2-byte length field (includes header and data)
     - 4-byte operation code 
     - Optional data block (for write operations)
   - Handles network byte order conversion
   - Returns `true` on successful send, `false` otherwise

2. **recv_packet(int fd, uint32_t \*op, uint16_t \*ret, uint8_t \*block)**
   - Receives and unpacks packets from the server
   - Extracts operation code, return value, and optional data block
   - Handles network to host byte order conversion
   - Returns `true` on successful receive, `false` otherwise

3. **nread(int fd, int len, uint8_t \*buf)**
   - Utility function to read exactly `len` bytes from file descriptor `fd`
   - Returns `true` on success (all bytes read), `false` otherwise

4. **nwrite(int fd, int len, uint8_t \*buf)**
   - Utility function to write exactly `len` bytes to file descriptor `fd`
   - Returns `true` on success (all bytes written), `false` otherwise

## Packet Format

### Request Packet
```
+----------------+------------------+----------------------+
|   Length (2B)  | Operation (4B)   |   Data (optional)    |
+----------------+------------------+----------------------+
```

### Response Packet
```
+----------------+------------------+------------------+----------------------+
|   Length (2B)  | Operation (4B)   | Return Code (2B) |   Data (optional)    |
+----------------+------------------+------------------+----------------------+
```

## Building the Project
To build the project:
```bash
make
```

## Testing
The project includes test files in the `traces/` directory for verifying the correctness of the implementation.

## Usage Example
```c
#include "jbod.h"
#include "mdadm.h"
#include <stdio.h>

int main() {
  // Connect to JBOD server
  if (!jbod_connect("127.0.0.1", 8000)) {
    fprintf(stderr, "Failed to connect to server\n");
    return 1;
  }
  
  // Perform operations
  uint8_t block[JBOD_BLOCK_SIZE];
  uint32_t op = /* construct operation */;
  int ret = jbod_client_operation(op, block);
  
  // Disconnect when done
  jbod_disconnect();
  
  return 0;
}
```

## Contributors
This project was developed as part of CMPSC 311 course assignments (1-5).

## License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
