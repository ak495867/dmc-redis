# Demonic Redis (`dmc-redis`)

<p align="center">
  <img src="assets/logo.png" alt="Demonic Redis Logo" width="220">
</p>

<p align="center">
  <strong>An ultra-fast in-memory key-value and SIMD vector database server built 100% in pure Demonic C.</strong>
</p>

---

## ⚡ Overview

`dmc-redis` is an in-memory key-value store and 128-bit hardware SIMD vector search engine built natively with Demonic C. It implements the REdis Serialization Protocol (RESP), allowing native compatibility with `redis-cli`, Python `redis`, Node.js `ioredis`, and standard tooling.

### Highlights

- **RESP Compatible Protocol**: Speaks standard Redis protocol (`+OK`, `-ERR`, `:int`, `$bulk`).
- **Hardware SIMD Vector Search**: Custom `SIMD.SET` and `SIMD.SEARCH` commands lowering to 128-bit SSE/AVX registers for instantaneous vector dot products.
- **Microsecond In-Memory Store**: `PING`, `SET`, `GET`, `DEL`, `EXISTS`, `INCR`, `DECR`, `EXPIRE`, `TTL`.
- **Pure Demonic C**: Compiles directly with `dmc` or runs instantly via the build cache.
- **Zero Dependencies**: Standard ANSI C runtime and OS socket layer.

---

## 🚀 Quick Start

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
