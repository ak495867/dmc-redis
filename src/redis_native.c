#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
typedef int socklen_t;
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#define SOCKET int
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket close
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <immintrin.h>

static int _dmc_net_init_done = 0;
static void _dmc_net_init(void) {
#ifdef _WIN32
    if (!_dmc_net_init_done) {
        WSADATA wsa;
        WSAStartup(MAKEWORD(2, 2), &wsa);
        _dmc_net_init_done = 1;
    }
#endif
}

int dmc_tcp_listen(int port) {
    _dmc_net_init();
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET) return -1;
    int opt = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons((unsigned short)port);
    if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        closesocket(s);
        return -1;
    }
    if (listen(s, 512) == SOCKET_ERROR) {
        closesocket(s);
        return -1;
    }
    return (int)s;
}

int dmc_tcp_accept(int server_fd) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    SOCKET client = accept((SOCKET)server_fd, (struct sockaddr*)&client_addr, &client_len);
    if (client == INVALID_SOCKET) return -1;
    return (int)client;
}

static char _recv_buf[65536];
const char* dmc_tcp_read_request(int client_fd) {
    if (client_fd < 0) return NULL;
    int bytes = recv((SOCKET)client_fd, _recv_buf, sizeof(_recv_buf) - 1, 0);
    if (bytes <= 0) return NULL;
    _recv_buf[bytes] = '\0';
    return _recv_buf;
}

int dmc_tcp_send(int client_fd, const char* buffer, int len) {
    if (client_fd < 0 || !buffer || len <= 0) return -1;
    int total_sent = 0;
    while (total_sent < len) {
        int sent = send((SOCKET)client_fd, buffer + total_sent, len - total_sent, 0);
        if (sent <= 0) return -1;
        total_sent += sent;
    }
    return total_sent;
}

int dmc_tcp_close(int fd) {
    if (fd < 0) return -1;
    return closesocket((SOCKET)fd);
}

typedef struct DictEntry {
    char* key;
    char* val;
    long long expire_at;
    uint32_t access_time;
    struct DictEntry* next;
} DictEntry;

#define HASH_BUCKETS 16384
#define MAX_MEMORY_BYTES (64 * 1024 * 1024)

static DictEntry* _hash_table[HASH_BUCKETS];
static int _hash_table_count = 0;
static size_t _current_memory_usage = 0;

static uint32_t _hash_key(const char* key) {
    uint32_t hash = 2166136261u;
    while (*key) {
        hash ^= (uint8_t)(*key++);
        hash *= 16777619u;
    }
    return hash % HASH_BUCKETS;
}

static long long _current_time_sec(void) {
    return (long long)time(NULL);
}

static uint32_t _current_clock(void) {
    return (uint32_t)clock();
}

static void _evict_expired(void) {
    static int check_counter = 0;
    if ((++check_counter & 0x3F) != 0) return;
    long long now = _current_time_sec();
    static int last_bucket = 0;
    for (int k = 0; k < 16; ++k) {
        int i = (last_bucket + k) % HASH_BUCKETS;
        DictEntry** curr = &_hash_table[i];
        while (*curr) {
            DictEntry* entry = *curr;
            if (entry->expire_at > 0 && entry->expire_at < now) {
                *curr = entry->next;
                _current_memory_usage -= sizeof(DictEntry) + strlen(entry->key) + 1 + strlen(entry->val) + 1;
                free(entry->key);
                free(entry->val);
                free(entry);
                _hash_table_count--;
            } else {
                curr = &entry->next;
            }
        }
    }
    last_bucket = (last_bucket + 16) % HASH_BUCKETS;
}

static void _evict_lru_one(void) {
    uint32_t oldest_time = 0xFFFFFFFF;
    DictEntry* oldest_entry = NULL;
    DictEntry** oldest_prev = NULL;

    for (int i = 0; i < HASH_BUCKETS; ++i) {
        DictEntry** curr = &_hash_table[i];
        while (*curr) {
            DictEntry* entry = *curr;
            if (entry->access_time <= oldest_time) {
                oldest_time = entry->access_time;
                oldest_entry = entry;
                oldest_prev = curr;
            }
            curr = &entry->next;
        }
    }

    if (oldest_entry && oldest_prev) {
        *oldest_prev = oldest_entry->next;
        _current_memory_usage -= sizeof(DictEntry) + strlen(oldest_entry->key) + 1 + strlen(oldest_entry->val) + 1;
        free(oldest_entry->key);
        free(oldest_entry->val);
        free(oldest_entry);
        _hash_table_count--;
    }
}

int store_set(const char* key, const char* val) {
    if (!key || !val) return 0;
    _evict_expired();

    size_t key_len = strlen(key);
    size_t val_len = strlen(val);
    size_t needed = sizeof(DictEntry) + key_len + 1 + val_len + 1;

    uint32_t bucket = _hash_key(key);
    DictEntry* curr = _hash_table[bucket];
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            size_t old_val_len = strlen(curr->val);
            char* new_val = (char*)malloc(val_len + 1);
            if (!new_val) return 0;
            memcpy(new_val, val, val_len + 1);
            free(curr->val);
            curr->val = new_val;
            curr->expire_at = 0;
            curr->access_time = _current_clock();
            _current_memory_usage = _current_memory_usage - old_val_len + val_len;
            return 1;
        }
        curr = curr->next;
    }

    while (_current_memory_usage + needed > MAX_MEMORY_BYTES && _hash_table_count > 0) {
        _evict_lru_one();
    }

    DictEntry* entry = (DictEntry*)malloc(sizeof(DictEntry));
    if (!entry) return 0;
    entry->key = (char*)malloc(key_len + 1);
    entry->val = (char*)malloc(val_len + 1);
    if (!entry->key || !entry->val) {
        if (entry->key) free(entry->key);
        if (entry->val) free(entry->val);
        free(entry);
        return 0;
    }

    memcpy(entry->key, key, key_len + 1);
    memcpy(entry->val, val, val_len + 1);
    entry->expire_at = 0;
    entry->access_time = _current_clock();
    entry->next = _hash_table[bucket];
    _hash_table[bucket] = entry;
    _hash_table_count++;
    _current_memory_usage += needed;

    return 1;
}

const char* store_get(const char* key) {
    if (!key) return NULL;
    uint32_t bucket = _hash_key(key);
    long long now = _current_time_sec();
    DictEntry** curr = &_hash_table[bucket];
    while (*curr) {
        DictEntry* entry = *curr;
        if (strcmp(entry->key, key) == 0) {
            if (entry->expire_at > 0 && entry->expire_at < now) {
                *curr = entry->next;
                _current_memory_usage -= sizeof(DictEntry) + strlen(entry->key) + 1 + strlen(entry->val) + 1;
                free(entry->key);
                free(entry->val);
                free(entry);
                _hash_table_count--;
                return NULL;
            }
            entry->access_time = _current_clock();
            return entry->val;
        }
        curr = &entry->next;
    }
    return NULL;
}

int store_del(const char* key) {
    if (!key) return 0;
    uint32_t bucket = _hash_key(key);
    DictEntry** curr = &_hash_table[bucket];
    while (*curr) {
        DictEntry* entry = *curr;
        if (strcmp(entry->key, key) == 0) {
            *curr = entry->next;
            _current_memory_usage -= sizeof(DictEntry) + strlen(entry->key) + 1 + strlen(entry->val) + 1;
            free(entry->key);
            free(entry->val);
            free(entry);
            _hash_table_count--;
            return 1;
        }
        curr = &entry->next;
    }
    return 0;
}

int store_exists(const char* key) {
    return store_get(key) != NULL ? 1 : 0;
}

int store_expire(const char* key, int seconds) {
    if (!key) return 0;
    uint32_t bucket = _hash_key(key);
    DictEntry* curr = _hash_table[bucket];
    while (curr) {
        if (strcmp(curr->key, key) == 0) {
            curr->expire_at = _current_time_sec() + seconds;
            return 1;
        }
        curr = curr->next;
    }
    return 0;
}

int store_ttl(const char* key) {
    if (!key) return -2;
    uint32_t bucket = _hash_key(key);
    long long now = _current_time_sec();
    DictEntry** curr = &_hash_table[bucket];
    while (*curr) {
        DictEntry* entry = *curr;
        if (strcmp(entry->key, key) == 0) {
            if (entry->expire_at > 0 && entry->expire_at < now) {
                *curr = entry->next;
                _current_memory_usage -= sizeof(DictEntry) + strlen(entry->key) + 1 + strlen(entry->val) + 1;
                free(entry->key);
                free(entry->val);
                free(entry);
                _hash_table_count--;
                return -2;
            }
            if (entry->expire_at == 0) return -1;
            long long rem = entry->expire_at - now;
            return rem > 0 ? (int)rem : -2;
        }
        curr = &entry->next;
    }
    return -2;
}

int store_incr(const char* key, int delta) {
    const char* val = store_get(key);
    int num = 0;
    if (val) {
        num = atoi(val);
    }
    num += delta;
    char buf[64];
    snprintf(buf, sizeof(buf), "%d", num);
    store_set(key, buf);
    return num;
}

#define MAX_VECTORS 4096

typedef struct {
    char key[64];
    float __attribute__((aligned(16))) components[4];
    int occupied;
} VectorEntry;

static VectorEntry _dmc_vectors[MAX_VECTORS];
static int _dmc_vector_count = 0;

int simd_store_set(const char* key, float x, float y, float z, float w) {
    if (!key) return 0;
    for (int i = 0; i < MAX_VECTORS; ++i) {
        if (_dmc_vectors[i].occupied && strcmp(_dmc_vectors[i].key, key) == 0) {
            _dmc_vectors[i].components[0] = x;
            _dmc_vectors[i].components[1] = y;
            _dmc_vectors[i].components[2] = z;
            _dmc_vectors[i].components[3] = w;
            return 1;
        }
    }
    for (int i = 0; i < MAX_VECTORS; ++i) {
        if (!_dmc_vectors[i].occupied) {
            strncpy(_dmc_vectors[i].key, key, sizeof(_dmc_vectors[i].key) - 1);
            _dmc_vectors[i].key[sizeof(_dmc_vectors[i].key) - 1] = '\0';
            _dmc_vectors[i].components[0] = x;
            _dmc_vectors[i].components[1] = y;
            _dmc_vectors[i].components[2] = z;
            _dmc_vectors[i].components[3] = w;
            _dmc_vectors[i].occupied = 1;
            _dmc_vector_count++;
            return 1;
        }
    }
    return 0;
}

const char* simd_store_search(float qx, float qy, float qz, float qw) {
    int best_idx = -1;
    float best_dot = -1e9f;
    __m128 q = _mm_set_ps(qw, qz, qy, qx);

    for (int i = 0; i < MAX_VECTORS; ++i) {
        if (_dmc_vectors[i].occupied) {
            __m128 v = _mm_load_ps(_dmc_vectors[i].components);
            __m128 prod = _mm_mul_ps(q, v);
            __m128 shuf = _mm_shuffle_ps(prod, prod, _MM_SHUFFLE(2, 3, 0, 1));
            __m128 sums = _mm_add_ps(prod, shuf);
            shuf = _mm_movehl_ps(shuf, sums);
            sums = _mm_add_ss(sums, shuf);
            float dot;
            _mm_store_ss(&dot, sums);

            if (dot > best_dot) {
                best_dot = dot;
                best_idx = i;
            }
        }
    }

    if (best_idx >= 0) {
        return _dmc_vectors[best_idx].key;
    }
    return "";
}

int simd_store_count(void) {
    return _dmc_vector_count;
}

static char _cmd_resp_buf[131072];

static int parse_resp_or_inline(const char* raw_cmd, int raw_len, char* tokens[], int max_tokens, int* out_bytes_consumed) {
    if (!raw_cmd || raw_len <= 0) {
        if (out_bytes_consumed) *out_bytes_consumed = 0;
        return 0;
    }

    const char* ptr = raw_cmd;
    const char* end = raw_cmd + raw_len;

    while (ptr < end && (*ptr == ' ' || *ptr == '\r' || *ptr == '\n')) {
        ptr++;
    }
    if (ptr >= end) {
        if (out_bytes_consumed) *out_bytes_consumed = (int)(ptr - raw_cmd);
        return 0;
    }

    if (*ptr == '*') {
        ptr++;
        int num_args = 0;
        while (ptr < end && *ptr >= '0' && *ptr <= '9') {
            num_args = num_args * 10 + (*ptr - '0');
            ptr++;
        }
        if (ptr + 1 >= end || ptr[0] != '\r' || ptr[1] != '\n') {
            if (out_bytes_consumed) *out_bytes_consumed = 0;
            return 0;
        }
        ptr += 2;

        int count = 0;
        for (int i = 0; i < num_args; ++i) {
            if (ptr >= end || *ptr != '$') {
                if (out_bytes_consumed) *out_bytes_consumed = 0;
                return 0;
            }
            ptr++;
            int bulk_len = 0;
            while (ptr < end && *ptr >= '0' && *ptr <= '9') {
                bulk_len = bulk_len * 10 + (*ptr - '0');
                ptr++;
            }
            if (ptr + 1 >= end || ptr[0] != '\r' || ptr[1] != '\n') {
                if (out_bytes_consumed) *out_bytes_consumed = 0;
                return 0;
            }
            ptr += 2;

            if (ptr + bulk_len + 2 > end) {
                if (out_bytes_consumed) *out_bytes_consumed = 0;
                return 0;
            }

            if (count < max_tokens) {
                char* tok = (char*)malloc(bulk_len + 1);
                if (tok) {
                    memcpy(tok, ptr, bulk_len);
                    tok[bulk_len] = '\0';
                    tokens[count++] = tok;
                }
            }
            ptr += bulk_len;
            if (ptr[0] == '\r' && ptr[1] == '\n') {
                ptr += 2;
            }
        }
        if (out_bytes_consumed) *out_bytes_consumed = (int)(ptr - raw_cmd);
        return count;
    }

    const char* line_end = ptr;
    while (line_end < end && *line_end != '\n') {
        line_end++;
    }
    if (line_end < end && *line_end == '\n') {
        int line_len = (int)(line_end - ptr);
        char* line_copy = (char*)malloc(line_len + 1);
        if (!line_copy) {
            if (out_bytes_consumed) *out_bytes_consumed = (int)(line_end + 1 - raw_cmd);
            return 0;
        }
        memcpy(line_copy, ptr, line_len);
        line_copy[line_len] = '\0';

        int count = 0;
        char* p = line_copy;
        while (*p && count < max_tokens) {
            while (*p == ' ' || *p == '\r' || *p == '\n') *p++ = '\0';
            if (!*p) break;
            if (*p == 34) {
                p++;
                char* start = p;
                while (*p && *p != 34) p++;
                if (*p == 34) *p++ = '\0';
                tokens[count++] = strdup(start);
            } else {
                char* start = p;
                while (*p && *p != ' ' && *p != '\r' && *p != '\n') p++;
                if (*p) *p++ = '\0';
                tokens[count++] = strdup(start);
            }
        }
        free(line_copy);
        if (out_bytes_consumed) *out_bytes_consumed = (int)(line_end + 1 - raw_cmd);
        return count;
    }

    if (out_bytes_consumed) *out_bytes_consumed = 0;
    return 0;
}

const char* execute_redis_command(const char* raw_cmd) {
    if (!raw_cmd || !*raw_cmd) return "+OK\r\n";
    _evict_expired();

    char* tokens[16];
    int consumed = 0;
    int count = parse_resp_or_inline(raw_cmd, (int)strlen(raw_cmd), tokens, 16, &consumed);
    if (count == 0) return "+OK\r\n";

    char cmd[64];
    strncpy(cmd, tokens[0], sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    for (int i = 0; cmd[i]; ++i) {
        if (cmd[i] >= 'a' && cmd[i] <= 'z') cmd[i] -= 32;
    }

    const char* result = "-ERR unknown command\r\n";

    if (strcmp(cmd, "PING") == 0) {
        if (count > 1) {
            snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(tokens[1]), tokens[1]);
            result = _cmd_resp_buf;
        } else {
            result = "+PONG\r\n";
        }
    } else if (strcmp(cmd, "SET") == 0) {
        if (count < 3) {
            result = "-ERR wrong number of arguments for 'set' command\r\n";
        } else {
            store_set(tokens[1], tokens[2]);
            if (count >= 5 && (strcmp(tokens[3], "EX") == 0 || strcmp(tokens[3], "ex") == 0)) {
                store_expire(tokens[1], atoi(tokens[4]));
            }
            result = "+OK\r\n";
        }
    } else if (strcmp(cmd, "GET") == 0) {
        if (count < 2) {
            result = "-ERR wrong number of arguments for 'get' command\r\n";
        } else {
            const char* val = store_get(tokens[1]);
            if (!val) {
                result = "$-1\r\n";
            } else {
                snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(val), val);
                result = _cmd_resp_buf;
            }
        }
    } else if (strcmp(cmd, "DEL") == 0) {
        if (count < 2) {
            result = "-ERR wrong number of arguments for 'del' command\r\n";
        } else {
            int deleted = 0;
            for (int i = 1; i < count; ++i) {
                if (store_del(tokens[i])) deleted++;
            }
            snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", deleted);
            result = _cmd_resp_buf;
        }
    } else if (strcmp(cmd, "EXISTS") == 0) {
        if (count < 2) {
            result = "-ERR wrong number of arguments for 'exists' command\r\n";
        } else {
            int exists = 0;
            for (int i = 1; i < count; ++i) {
                if (store_exists(tokens[i])) exists++;
            }
            snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", exists);
            result = _cmd_resp_buf;
        }
    } else if (strcmp(cmd, "INCR") == 0) {
        if (count < 2) {
            result = "-ERR wrong number of arguments for 'incr' command\r\n";
        } else {
            int res = store_incr(tokens[1], 1);
            snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
            result = _cmd_resp_buf;
        }
    } else if (strcmp(cmd, "DECR") == 0) {
        if (count < 2) {
            result = "-ERR wrong number of arguments for 'decr' command\r\n";
        } else {
            int res = store_incr(tokens[1], -1);
            snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
            result = _cmd_resp_buf;
        }
    } else if (strcmp(cmd, "EXPIRE") == 0) {
        if (count < 3) {
            result = "-ERR wrong number of arguments for 'expire' command\r\n";
        } else {
            int res = store_expire(tokens[1], atoi(tokens[2]));
            snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
            result = _cmd_resp_buf;
        }
    } else if (strcmp(cmd, "TTL") == 0) {
        if (count < 2) {
            result = "-ERR wrong number of arguments for 'ttl' command\r\n";
        } else {
            int res = store_ttl(tokens[1]);
            snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
            result = _cmd_resp_buf;
        }
    } else if (strcmp(cmd, "SIMD.SET") == 0) {
        if (count < 6) {
            result = "-ERR usage: SIMD.SET <key> <x> <y> <z> <w>\r\n";
        } else {
            float x = (float)atof(tokens[2]);
            float y = (float)atof(tokens[3]);
            float z = (float)atof(tokens[4]);
            float w = (float)atof(tokens[5]);
            simd_store_set(tokens[1], x, y, z, w);
            result = "+OK\r\n";
        }
    } else if (strcmp(cmd, "SIMD.SEARCH") == 0) {
        if (count < 5) {
            result = "-ERR usage: SIMD.SEARCH <x> <y> <z> <w>\r\n";
        } else {
            float x = (float)atof(tokens[1]);
            float y = (float)atof(tokens[2]);
            float z = (float)atof(tokens[3]);
            float w = (float)atof(tokens[4]);
            const char* best = simd_store_search(x, y, z, w);
            if (!best || !*best) {
                result = "$-1\r\n";
            } else {
                snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(best), best);
                result = _cmd_resp_buf;
            }
        }
    } else if (strcmp(cmd, "INFO") == 0) {
        char info[512];
        snprintf(info, sizeof(info),
                 "redis_version:1.0.0-demonic\r\n"
                 "demoniac_engine:dmc-c11\r\n"
                 "arch:x86_64\r\n"
                 "tcp_port:6379\r\n"
                 "simd_enabled:1\r\n"
                 "used_memory:%zu\r\n"
                 "max_memory:%d\r\n"
                 "keys_count:%d\r\n",
                 _current_memory_usage, MAX_MEMORY_BYTES, _hash_table_count);
        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(info), info);
        result = _cmd_resp_buf;
    }

    for (int i = 0; i < count; ++i) {
        if (tokens[i]) free(tokens[i]);
    }

    return result;
}

int dmc_tcp_process_client(int client_fd) {
    if (client_fd < 0) return 0;
    char buffer[65536];
    int buf_len = 0;

    while (1) {
        int bytes = recv((SOCKET)client_fd, buffer + buf_len, sizeof(buffer) - 1 - buf_len, 0);
        if (bytes <= 0) {
            break;
        }
        buf_len += bytes;
        buffer[buf_len] = '\0';

        while (buf_len > 0) {
            char* tokens[16];
            int consumed = 0;
            int count = parse_resp_or_inline(buffer, buf_len, tokens, 16, &consumed);
            if (consumed <= 0) {
                break;
            }

            if (count > 0) {
                char cmd[64];
                strncpy(cmd, tokens[0], sizeof(cmd) - 1);
                cmd[sizeof(cmd) - 1] = '\0';
                for (int i = 0; cmd[i]; ++i) {
                    if (cmd[i] >= 'a' && cmd[i] <= 'z') cmd[i] -= 32;
                }

                const char* result = "-ERR unknown command\r\n";

                if (strcmp(cmd, "PING") == 0) {
                    if (count > 1) {
                        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(tokens[1]), tokens[1]);
                        result = _cmd_resp_buf;
                    } else {
                        result = "+PONG\r\n";
                    }
                } else if (strcmp(cmd, "SET") == 0) {
                    if (count < 3) {
                        result = "-ERR wrong number of arguments for 'set' command\r\n";
                    } else {
                        store_set(tokens[1], tokens[2]);
                        if (count >= 5 && (strcmp(tokens[3], "EX") == 0 || strcmp(tokens[3], "ex") == 0)) {
                            store_expire(tokens[1], atoi(tokens[4]));
                        }
                        result = "+OK\r\n";
                    }
                } else if (strcmp(cmd, "GET") == 0) {
                    if (count < 2) {
                        result = "-ERR wrong number of arguments for 'get' command\r\n";
                    } else {
                        const char* val = store_get(tokens[1]);
                        if (!val) {
                            result = "$-1\r\n";
                        } else {
                            snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(val), val);
                            result = _cmd_resp_buf;
                        }
                    }
                } else if (strcmp(cmd, "DEL") == 0) {
                    if (count < 2) {
                        result = "-ERR wrong number of arguments for 'del' command\r\n";
                    } else {
                        int deleted = 0;
                        for (int i = 1; i < count; ++i) {
                            if (store_del(tokens[i])) deleted++;
                        }
                        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", deleted);
                        result = _cmd_resp_buf;
                    }
                } else if (strcmp(cmd, "EXISTS") == 0) {
                    if (count < 2) {
                        result = "-ERR wrong number of arguments for 'exists' command\r\n";
                    } else {
                        int exists = 0;
                        for (int i = 1; i < count; ++i) {
                            if (store_exists(tokens[i])) exists++;
                        }
                        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", exists);
                        result = _cmd_resp_buf;
                    }
                } else if (strcmp(cmd, "INCR") == 0) {
                    if (count < 2) {
                        result = "-ERR wrong number of arguments for 'incr' command\r\n";
                    } else {
                        int res = store_incr(tokens[1], 1);
                        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
                        result = _cmd_resp_buf;
                    }
                } else if (strcmp(cmd, "DECR") == 0) {
                    if (count < 2) {
                        result = "-ERR wrong number of arguments for 'decr' command\r\n";
                    } else {
                        int res = store_incr(tokens[1], -1);
                        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
                        result = _cmd_resp_buf;
                    }
                } else if (strcmp(cmd, "EXPIRE") == 0) {
                    if (count < 3) {
                        result = "-ERR wrong number of arguments for 'expire' command\r\n";
                    } else {
                        int res = store_expire(tokens[1], atoi(tokens[2]));
                        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
                        result = _cmd_resp_buf;
                    }
                } else if (strcmp(cmd, "TTL") == 0) {
                    if (count < 2) {
                        result = "-ERR wrong number of arguments for 'ttl' command\r\n";
                    } else {
                        int res = store_ttl(tokens[1]);
                        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
                        result = _cmd_resp_buf;
                    }
                } else if (strcmp(cmd, "SIMD.SET") == 0) {
                    if (count < 6) {
                        result = "-ERR usage: SIMD.SET <key> <x> <y> <z> <w>\r\n";
                    } else {
                        float x = (float)atof(tokens[2]);
                        float y = (float)atof(tokens[3]);
                        float z = (float)atof(tokens[4]);
                        float w = (float)atof(tokens[5]);
                        simd_store_set(tokens[1], x, y, z, w);
                        result = "+OK\r\n";
                    }
                } else if (strcmp(cmd, "SIMD.SEARCH") == 0) {
                    if (count < 5) {
                        result = "-ERR usage: SIMD.SEARCH <x> <y> <z> <w>\r\n";
                    } else {
                        float x = (float)atof(tokens[1]);
                        float y = (float)atof(tokens[2]);
                        float z = (float)atof(tokens[3]);
                        float w = (float)atof(tokens[4]);
                        const char* best = simd_store_search(x, y, z, w);
                        if (!best || !*best) {
                            result = "$-1\r\n";
                        } else {
                            snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(best), best);
                            result = _cmd_resp_buf;
                        }
                    }
                } else if (strcmp(cmd, "INFO") == 0) {
                    char info[512];
                    snprintf(info, sizeof(info),
                             "redis_version:1.0.0-demonic\r\n"
                             "demoniac_engine:dmc-c11\r\n"
                             "arch:x86_64\r\n"
                             "tcp_port:6379\r\n"
                             "simd_enabled:1\r\n"
                             "used_memory:%zu\r\n"
                             "max_memory:%d\r\n"
                             "keys_count:%d\r\n",
                             _current_memory_usage, MAX_MEMORY_BYTES, _hash_table_count);
                    snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(info), info);
                    result = _cmd_resp_buf;
                }

                for (int i = 0; i < count; ++i) {
                    if (tokens[i]) free(tokens[i]);
                }

                dmc_tcp_send(client_fd, result, (int)strlen(result));
            }

            int remaining = buf_len - consumed;
            if (remaining > 0) {
                memmove(buffer, buffer + consumed, remaining);
            }
            buf_len = remaining;
            buffer[buf_len] = '\0';
        }
    }

    dmc_tcp_close(client_fd);
    return 1;
}

static char _resp_buf[65536];

const char* resp_simple_string(const char* str) {
    snprintf(_resp_buf, sizeof(_resp_buf), "+%s\r\n", str ? str : "");
    return _resp_buf;
}

const char* resp_error(const char* err) {
    snprintf(_resp_buf, sizeof(_resp_buf), "-ERR %s\r\n", err ? err : "unknown error");
    return _resp_buf;
}

const char* resp_integer(int val) {
    snprintf(_resp_buf, sizeof(_resp_buf), ":%d\r\n", val);
    return _resp_buf;
}

const char* resp_bulk_string(const char* str) {
    if (!str) {
        return "$-1\r\n";
    }
    int len = (int)strlen(str);
    snprintf(_resp_buf, sizeof(_resp_buf), "$%d\r\n%s\r\n", len, str);
    return _resp_buf;
}

const char* resp_null(void) {
    return "$-1\r\n";
}

const char* resp_ok(void) {
    return "+OK\r\n";
}

const char* resp_pong(void) {
    return "+PONG\r\n";
}
