# Demonic Redis (`dmc-redis`)

<p align="center">
  <img src="assets/logo.png" alt="Demonic Redis Logo" width="220">
</p>

<p align="center">
  <strong>An ultra-fast in-memory key-value and 128-bit hardware SIMD vector database server built in Demonic C.</strong>
</p>

<p align="center">
  <a href="https://github.com/ak495867/dmc-redis/releases"><img src="https://img.shields.io/github/v/release/ak495867/dmc-redis?style=for-the-badge&color=blue" alt="Release"></a>
  <a href="https://github.com/ak495867/dmc-redis/actions"><img src="https://img.shields.io/github/actions/workflow/status/ak495867/dmc-redis/ci.yml?style=for-the-badge" alt="CI"></a>
  <a href="LICENSE"><img src="https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge" alt="License"></a>
</p>

---

## ⚡ Overview

`dmc-redis` is an in-memory key-value store and 128-bit hardware SIMD vector search engine built natively with Demonic C. It implements the REdis Serialization Protocol (RESP), allowing native compatibility with `redis-cli`, Python `redis`, Node.js `ioredis`, and standard tooling.

### Highlights

- **RESP & Inline Compatibility**: Full support for Redis Serialization Protocol arrays (`*<len>...$<len>`) and raw inline commands.
- **Hardware SIMD Vector Search**: Custom `SIMD.SET` and `SIMD.SEARCH` commands lowering to 128-bit SSE registers for instantaneous vector dot products.
- **O(1) Hash Table Indexing**: FNV-1a hash bucket store with open chaining replacing linear scans.
- **Persistent Keep-Alive Sockets**: Full connection reuse and request pipelining for SDK connection pools.
- **Active TTL & LRU Eviction**: Automatic memory sweeps with clock-based LRU memory cap enforcement.
- **Zero Dependencies**: Native ANSI C socket layer with no runtime bloat.

---

## ⚡ Direct Install

### Linux & macOS (bash / zsh)
```bash
curl -fsSL https://raw.githubusercontent.com/ak495867/dmc-redis/master/install.sh | sh
```

### Windows (PowerShell)
```powershell
irm https://raw.githubusercontent.com/ak495867/dmc-redis/master/install.ps1 | iex
```

---

## 🚀 Quick Start (From Source)

### 1. Run Directly with `dmc`
```bash
dmc src/main.dmc src/redis_native.c -lws2_32
```

### 2. Build Native Binary
```bash
dmc build src/main.dmc src/redis_native.c -o bin/dmc-redis.exe -lws2_32
./bin/dmc-redis.exe
```

---

## 🧪 Testing

Run internal unit and integration test suite:
```bash
dmc test/test_redis.dmc src/redis_native.c -lws2_32
```

Output:
```text
4/4 tests passed
```

---

## 💻 Redis CLI & SDK Usage

Connect with any standard client:
```bash
redis-cli -p 6379
```

```text
127.0.0.1:6379> PING
PONG
127.0.0.1:6379> SET demonic "speed"
OK
127.0.0.1:6379> GET demonic
"speed"
127.0.0.1:6379> INCR visits
1
127.0.0.1:6379> INCR visits
2
127.0.0.1:6379> EXPIRE visits 60
1
127.0.0.1:6379> TTL visits
59
```

### SIMD Vector Commands
```text
127.0.0.1:6379> SIMD.SET v1 1.0 0.0 0.0 0.0
OK
127.0.0.1:6379> SIMD.SET v2 0.0 1.0 0.0 0.0
OK
127.0.0.1:6379> SIMD.SEARCH 0.95 0.05 0.0 0.0
"v1"
```

---

## 📜 License

MIT License. Crafted with Demonic C.
