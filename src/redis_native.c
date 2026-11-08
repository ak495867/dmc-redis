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
    if (listen(s, 128) == SOCKET_ERROR) {
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

static char _recv_buf[4096];
const char* dmc_tcp_read_request(int client_fd) {
    if (client_fd < 0) return NULL;
    int bytes = recv((SOCKET)client_fd, _recv_buf, sizeof(_recv_buf) - 1, 0);
    if (bytes <= 0) return NULL;
    _recv_buf[bytes] = '\0';
    return _recv_buf;
}

int dmc_tcp_send(int client_fd, const char* buffer, int len) {
    if (client_fd < 0 || !buffer || len <= 0) return -1;
    return send((SOCKET)client_fd, buffer, len, 0);
}

int dmc_tcp_close(int fd) {
    if (fd < 0) return -1;
    return closesocket((SOCKET)fd);
}

#define MAX_ENTRIES 4096
#define MAX_KEY_LEN 128
#define MAX_VAL_LEN 4096

typedef struct {
    char key[MAX_KEY_LEN];
    char val[MAX_VAL_LEN];
    long long expire_at;
    int occupied;
} StoreEntry;

static StoreEntry _dmc_store[MAX_ENTRIES];
static int _dmc_store_count = 0;

static long long _current_time_sec(void) {
    return (long long)time(NULL);
}

int store_set(const char* key, const char* val) {
    if (!key || !val) return 0;
    long long now = _current_time_sec();
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        if (_dmc_store[i].occupied && strcmp(_dmc_store[i].key, key) == 0) {
            strncpy(_dmc_store[i].val, val, MAX_VAL_LEN - 1);
            _dmc_store[i].val[MAX_VAL_LEN - 1] = '\0';
            _dmc_store[i].expire_at = 0;
            return 1;
        }
    }
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        if (!_dmc_store[i].occupied || (_dmc_store[i].expire_at > 0 && _dmc_store[i].expire_at < now)) {
            strncpy(_dmc_store[i].key, key, MAX_KEY_LEN - 1);
            _dmc_store[i].key[MAX_KEY_LEN - 1] = '\0';
            strncpy(_dmc_store[i].val, val, MAX_VAL_LEN - 1);
            _dmc_store[i].val[MAX_VAL_LEN - 1] = '\0';
            _dmc_store[i].expire_at = 0;
            _dmc_store[i].occupied = 1;
            _dmc_store_count++;
            return 1;
        }
    }
    return 0;
}

const char* store_get(const char* key) {
    if (!key) return NULL;
    long long now = _current_time_sec();
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        if (_dmc_store[i].occupied && strcmp(_dmc_store[i].key, key) == 0) {
            if (_dmc_store[i].expire_at > 0 && _dmc_store[i].expire_at < now) {
                _dmc_store[i].occupied = 0;
                return NULL;
            }
            return _dmc_store[i].val;
        }
    }
    return NULL;
}

int store_del(const char* key) {
    if (!key) return 0;
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        if (_dmc_store[i].occupied && strcmp(_dmc_store[i].key, key) == 0) {
            _dmc_store[i].occupied = 0;
            return 1;
        }
    }
    return 0;
}

int store_exists(const char* key) {
    return store_get(key) != NULL ? 1 : 0;
}

int store_expire(const char* key, int seconds) {
    if (!key) return 0;
    long long now = _current_time_sec();
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        if (_dmc_store[i].occupied && strcmp(_dmc_store[i].key, key) == 0) {
            _dmc_store[i].expire_at = now + seconds;
            return 1;
        }
    }
    return 0;
}

int store_ttl(const char* key) {
    if (!key) return -2;
    long long now = _current_time_sec();
    for (int i = 0; i < MAX_ENTRIES; ++i) {
        if (_dmc_store[i].occupied && strcmp(_dmc_store[i].key, key) == 0) {
            if (_dmc_store[i].expire_at == 0) return -1;
            long long rem = _dmc_store[i].expire_at - now;
            return rem > 0 ? (int)rem : -2;
        }
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

#define MAX_VECTORS 1024
#define VEC_KEY_LEN 64

typedef struct {
    char key[VEC_KEY_LEN];
    float x;
    float y;
    float z;
    float w;
    int occupied;
} VectorEntry;

static VectorEntry _dmc_vectors[MAX_VECTORS];
static int _dmc_vector_count = 0;

int simd_store_set(const char* key, float x, float y, float z, float w) {
    if (!key) return 0;
    for (int i = 0; i < MAX_VECTORS; ++i) {
        if (_dmc_vectors[i].occupied && strcmp(_dmc_vectors[i].key, key) == 0) {
            _dmc_vectors[i].x = x;
            _dmc_vectors[i].y = y;
            _dmc_vectors[i].z = z;
            _dmc_vectors[i].w = w;
            return 1;
        }
    }
    for (int i = 0; i < MAX_VECTORS; ++i) {
        if (!_dmc_vectors[i].occupied) {
            strncpy(_dmc_vectors[i].key, key, VEC_KEY_LEN - 1);
            _dmc_vectors[i].key[VEC_KEY_LEN - 1] = '\0';
            _dmc_vectors[i].x = x;
            _dmc_vectors[i].y = y;
            _dmc_vectors[i].z = z;
            _dmc_vectors[i].w = w;
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
    for (int i = 0; i < MAX_VECTORS; ++i) {
        if (_dmc_vectors[i].occupied) {
            float dot = (qx * _dmc_vectors[i].x) +
                        (qy * _dmc_vectors[i].y) +
                        (qz * _dmc_vectors[i].z) +
                        (qw * _dmc_vectors[i].w);
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

static char _cmd_resp_buf[65536];

static int parse_tokens(char* line, char* tokens[], int max_tokens) {
    int count = 0;
    char* p = line;
    while (*p && count < max_tokens) {
        while (*p == ' ' || *p == '\r' || *p == '\n') *p++ = '\0';
        if (!*p) break;
        if (*p == 34) {
            p++;
            tokens[count++] = p;
            while (*p && *p != 34) p++;
            if (*p == 34) *p++ = '\0';
        } else {
            tokens[count++] = p;
            while (*p && *p != ' ' && *p != '\r' && *p != '\n') p++;
        }
    }
    return count;
}

const char* execute_redis_command(const char* raw_cmd) {
    if (!raw_cmd || !*raw_cmd) return "+OK\r\n";
    char line[4096];
    strncpy(line, raw_cmd, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    
    char* tokens[16];
    int count = parse_tokens(line, tokens, 16);
    if (count == 0) return "+OK\r\n";

    char cmd[64];
    strncpy(cmd, tokens[0], sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    for (int i = 0; cmd[i]; ++i) {
        if (cmd[i] >= 'a' && cmd[i] <= 'z') cmd[i] -= 32;
    }

    if (strcmp(cmd, "PING") == 0) {
        if (count > 1) {
            snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(tokens[1]), tokens[1]);
            return _cmd_resp_buf;
        }
        return "+PONG\r\n";
    }

    if (strcmp(cmd, "SET") == 0) {
        if (count < 3) return "-ERR wrong number of arguments for 'set' command\r\n";
        store_set(tokens[1], tokens[2]);
        if (count >= 5 && (strcmp(tokens[3], "EX") == 0 || strcmp(tokens[3], "ex") == 0)) {
            store_expire(tokens[1], atoi(tokens[4]));
        }
        return "+OK\r\n";
    }

    if (strcmp(cmd, "GET") == 0) {
        if (count < 2) return "-ERR wrong number of arguments for 'get' command\r\n";
        const char* val = store_get(tokens[1]);
        if (!val) return "$-1\r\n";
        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(val), val);
        return _cmd_resp_buf;
    }

    if (strcmp(cmd, "DEL") == 0) {
        if (count < 2) return "-ERR wrong number of arguments for 'del' command\r\n";
        int deleted = 0;
        for (int i = 1; i < count; ++i) {
            if (store_del(tokens[i])) deleted++;
        }
        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", deleted);
        return _cmd_resp_buf;
    }

    if (strcmp(cmd, "EXISTS") == 0) {
        if (count < 2) return "-ERR wrong number of arguments for 'exists' command\r\n";
        int exists = 0;
        for (int i = 1; i < count; ++i) {
            if (store_exists(tokens[i])) exists++;
        }
        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", exists);
        return _cmd_resp_buf;
    }

    if (strcmp(cmd, "INCR") == 0) {
        if (count < 2) return "-ERR wrong number of arguments for 'incr' command\r\n";
        int res = store_incr(tokens[1], 1);
        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
        return _cmd_resp_buf;
    }

    if (strcmp(cmd, "DECR") == 0) {
        if (count < 2) return "-ERR wrong number of arguments for 'decr' command\r\n";
        int res = store_incr(tokens[1], -1);
        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
        return _cmd_resp_buf;
    }

    if (strcmp(cmd, "EXPIRE") == 0) {
        if (count < 3) return "-ERR wrong number of arguments for 'expire' command\r\n";
        int res = store_expire(tokens[1], atoi(tokens[2]));
        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
        return _cmd_resp_buf;
    }

    if (strcmp(cmd, "TTL") == 0) {
        if (count < 2) return "-ERR wrong number of arguments for 'ttl' command\r\n";
        int res = store_ttl(tokens[1]);
        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), ":%d\r\n", res);
        return _cmd_resp_buf;
    }

    if (strcmp(cmd, "SIMD.SET") == 0) {
        if (count < 6) return "-ERR usage: SIMD.SET <key> <x> <y> <z> <w>\r\n";
        float x = (float)atof(tokens[2]);
        float y = (float)atof(tokens[3]);
        float z = (float)atof(tokens[4]);
        float w = (float)atof(tokens[5]);
        simd_store_set(tokens[1], x, y, z, w);
        return "+OK\r\n";
    }

    if (strcmp(cmd, "SIMD.SEARCH") == 0) {
        if (count < 5) return "-ERR usage: SIMD.SEARCH <x> <y> <z> <w>\r\n";
        float x = (float)atof(tokens[1]);
        float y = (float)atof(tokens[2]);
        float z = (float)atof(tokens[3]);
        float w = (float)atof(tokens[4]);
        const char* best = simd_store_search(x, y, z, w);
        if (!best || !*best) return "$-1\r\n";
        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(best), best);
        return _cmd_resp_buf;
    }

    if (strcmp(cmd, "INFO") == 0) {
        const char* info = "redis_version:1.0.0-demonic\r\ndemoniac_engine:dmc-c11\r\narch:x86_64\r\ntcp_port:6379\r\nsimd_enabled:1\r\n";
        snprintf(_cmd_resp_buf, sizeof(_cmd_resp_buf), "$%d\r\n%s\r\n", (int)strlen(info), info);
        return _cmd_resp_buf;
    }

    return "-ERR unknown command\r\n";
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
