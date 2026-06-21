# Reditto

A Redis-inspired, lightweight key-value store implemented in C++ for Linux environments. It uses TCP sockets for client-server communication and implements the RESP (Redis Serialization Protocol) for data serialization.

## Features

- **In-memory data store** with optional persistence to disk
- **RESP protocol** support for client communication
- **Multiple data structures**: Strings, Lists, and Hashes
- **TTL support** with automatic key expiration
- **Thread-safe** operations using mutex locks
- **Auto-persistence** with periodic data dumps

## Supported Data Structures

### Strings
Simple key-value pairs with string values.

### Lists
Ordered collections of strings supporting operations at both ends.

### Hashes
Maps between string field names and string values, perfect for representing objects.

## Commands Reference

### System Commands

| Command | Description | Example |
|---------|-------------|---------|
| `PING` | Test connectivity; returns PONG | `PING` |
| `ECHO <message>` | Echoes back the provided message | `ECHO hello world` |
| `FLUSHALL` | Removes all keys from the database | `FLUSHALL` |

### Key Commands

| Command | Description | Example |
|---------|-------------|---------|
| `SET <key> <value>` | Sets the value for a key | `SET name John` |
| `GET <key>` | Retrieves the value for a key | `GET name` |
| `KEYS` | Returns all keys in the database | `KEYS` |
| `TYPE <key>` | Returns the data type of a key | `TYPE name` |
| `DEL <key> [key ...]` | Deletes one or more keys | `DEL name age` |
| `EXPIRE <key> <seconds>` | Sets a TTL (time-to-live) for a key | `EXPIRE session 3600` |
| `RENAME <old_key> <new_key>` | Renames a key | `RENAME oldname newname` |

### List Commands

| Command | Description | Example |
|---------|-------------|---------|
| `LGET <key>` | Retrieves all elements in a list | `LGET mylist` |
| `LLEN <key>` | Returns the length of a list | `LLEN mylist` |
| `LPUSH <key> <value> [value ...]` | Pushes values to the head (left) of a list | `LPUSH mylist a b c` |
| `RPUSH <key> <value> [value ...]` | Pushes values to the tail (right) of a list | `RPUSH mylist 1 2 3` |
| `LPOP <key> <count>` | Removes and returns elements from the head | `LPOP mylist 2` |
| `RPOP <key> <count>` | Removes and returns elements from the tail | `RPOP mylist 1` |
| `LREM <key> <count> <value>` | Removes elements equal to value | `LREM mylist 2 x` |
| `LINDEX <key> <index>` | Returns element at specified index | `LINDEX mylist 0` |
| `LSET <key> <index> <value>` | Sets the value at specified index | `LSET mylist 0 newval` |

### Hash Commands

| Command | Description | Example |
|---------|-------------|---------|
| `HSET <key> <field> <value>` | Sets the value of a field in a hash | `HSET user name John` |
| `HGET <key> <field>` | Retrieves the value of a field | `HGET user name` |
| `HEXISTS <key> <field>` | Checks if a field exists in a hash | `HEXISTS user name` |
| `HDEL <key> <field>` | Deletes a field from a hash | `HDEL user name` |
| `HGETALL <key>` | Returns all fields and values in a hash | `HGETALL user` |
| `HKEYS <key>` | Returns all field names in a hash | `HKEYS user` |
| `HVALS <key>` | Returns all values in a hash | `HVALS user` |
| `HLEN <key>` | Returns the number of fields in a hash | `HLEN user` |
| `HMSET <key> <field1> <value1> [field2 value2 ...]` | Sets multiple fields at once | `HMSET user name John age 30` |

## Building

### Prerequisites

- GCC/G++ with C++17 support
- Make
- Linux environment

### Compile

```bash
make
```

This will compile the source files and produce the `v1` executable.

### Clean

```bash
make clean
```

## Running

Start the server on the default port (6379):

```bash
./v1
```

Or specify a custom port:

```bash
./v1 6380
```

The server will:
1. Load existing data from `my_redis.crdb` if the file exists
2. Start listening for client connections on the specified port
3. Automatically persist data to disk every 5 minutes

## Data Persistence

Data is automatically saved to `my_redis.crdb` every 5 minutes. The file uses a simple custom format:

```
K <key>:<value>
L <list_key> <elem1> <elem2> ...
H <hash_key> <field1>:<value1> <field2>:<value2> ...
```

### Example `my_redis.crdb`

```
K k1:v1
K k2:v2
L food f1 f2 f1 f3 f2 f0
L bevarges b4 b3 b2 b1 b9 b8 b7 b6
H nitems item3:c3 item2:c2 item4:c4 item1:c1
```

## Architecture

```
kv_store/v1/
├── inc/                    # Header files
│   ├── RedisServer.hpp     # Server class definition
│   ├── RedisDatabase.hpp   # Database singleton definition
│   ├── RedisCommandHandler.hpp  # Command processor definition
│   └── constants.hpp       # Global constants
├── lib/                    # Library utilities
│   ├── socket.lib.hpp      # TCP socket wrapper
│   ├── response.lib.hpp    # RESP response formatters
│   └── llist.lib.hpp       # Linked list implementation
├── src/                    # Source files
│   ├── main.cpp            # Entry point
│   ├── RedisServer.cpp     # Server implementation
│   ├── RedisDatabase.cpp   # Database implementation
│   └── RedisCommandHandler.cpp  # Command processing
├── obj/                    # Compiled object files (generated)
├── my_redis.crdb           # Persistence file (generated)
├── Makefile                # Build configuration
└── README.md               # This file
```

## Protocol

The server implements the RESP (Redis Serialization Protocol) for client communication. Commands can be sent in two formats:

1. **Inline format** (simple space-separated):
   ```
   SET key value
   ```

2. **RESP array format**:
   ```
   *3\r\n$3\r\nSET\r\n$3\r\nkey\r\n$5\r\nvalue\r\n
   ```

## Thread Safety

- All database operations are protected by a mutex (`db_mutex`)
- Key expiration is synchronized with each command execution via `syncExpiry()`
- A background thread handles periodic persistence without blocking client requests

## License

MIT License - see [LICENSE](LICENSE) file for details.

Copyright (c) 2026 Tushar-kukreti
