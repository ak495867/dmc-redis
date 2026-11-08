#define _GNU_SOURCE
#define _POSIX_C_SOURCE 200809L
#ifndef null
#define null ((void*)0)
#endif
typedef long long i64;
typedef unsigned long long u64;
typedef int i32;
typedef unsigned int u32;
typedef unsigned char u8;
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <io.h>
#include <direct.h>
#include <sys/timeb.h>
#define NSCLOSE closesocket
#else
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netdb.h>
#include <unistd.h>
#include <regex.h>
#define NSCLOSE close
#endif
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#if defined(_WIN32)
#define DMC_EXPORT __declspec(dllexport)
#elif defined(__GNUC__) || defined(__clang__)
#define DMC_EXPORT __attribute__((visibility("default")))
#else
#define DMC_EXPORT
#endif

/* Demonic C SIMD 128-bit Vector Extensions */
#if defined(__GNUC__) || defined(__clang__)
typedef float dmc_f32x4 __attribute__((vector_size(16)));
typedef int   dmc_i32x4 __attribute__((vector_size(16)));
#define DMC_SIMD_F32X4(a, b, c, d) ((dmc_f32x4){(float)(a), (float)(b), (float)(c), (float)(d)})
#define DMC_SIMD_I32X4(a, b, c, d) ((dmc_i32x4){(int)(a), (int)(b), (int)(c), (int)(d)})
#else
typedef union { float v[4]; } dmc_f32x4;
typedef union { int v[4]; }   dmc_i32x4;
static inline dmc_f32x4 DMC_SIMD_F32X4(float a, float b, float c, float d) { dmc_f32x4 r; r.v[0]=a; r.v[1]=b; r.v[2]=c; r.v[3]=d; return r; }
static inline dmc_i32x4 DMC_SIMD_I32X4(int a, int b, int c, int d) { dmc_i32x4 r; r.v[0]=a; r.v[1]=b; r.v[2]=c; r.v[3]=d; return r; }
#endif

static inline dmc_f32x4 dmc_f32x4_splat(float v) { return DMC_SIMD_F32X4(v, v, v, v); }
static inline dmc_i32x4 dmc_i32x4_splat(int v) { return DMC_SIMD_I32X4(v, v, v, v); }
static inline float dmc_f32x4_get(dmc_f32x4 v, int idx) {
#if defined(__GNUC__) || defined(__clang__)
    return ((float*)&v)[idx & 3];
#else
    return v.v[idx & 3];
#endif
}
static inline int dmc_i32x4_get(dmc_i32x4 v, int idx) {
#if defined(__GNUC__) || defined(__clang__)
    return ((int*)&v)[idx & 3];
#else
    return v.v[idx & 3];
#endif
}
static inline dmc_f32x4 dmc_f32x4_add(dmc_f32x4 a, dmc_f32x4 b) {
#if defined(__GNUC__) || defined(__clang__)
    return a + b;
#else
    dmc_f32x4 r; for(int i=0;i<4;i++) r.v[i]=a.v[i]+b.v[i]; return r;
#endif
}
static inline dmc_f32x4 dmc_f32x4_sub(dmc_f32x4 a, dmc_f32x4 b) {
#if defined(__GNUC__) || defined(__clang__)
    return a - b;
#else
    dmc_f32x4 r; for(int i=0;i<4;i++) r.v[i]=a.v[i]-b.v[i]; return r;
#endif
}
static inline dmc_f32x4 dmc_f32x4_mul(dmc_f32x4 a, dmc_f32x4 b) {
#if defined(__GNUC__) || defined(__clang__)
    return a * b;
#else
    dmc_f32x4 r; for(int i=0;i<4;i++) r.v[i]=a.v[i]*b.v[i]; return r;
#endif
}
static inline dmc_f32x4 dmc_f32x4_div(dmc_f32x4 a, dmc_f32x4 b) {
#if defined(__GNUC__) || defined(__clang__)
    return a / b;
#else
    dmc_f32x4 r; for(int i=0;i<4;i++) r.v[i]=a.v[i]/b.v[i]; return r;
#endif
}
static inline float dmc_f32x4_dot(dmc_f32x4 a, dmc_f32x4 b) {
    dmc_f32x4 m = dmc_f32x4_mul(a, b);
    return dmc_f32x4_get(m, 0) + dmc_f32x4_get(m, 1) + dmc_f32x4_get(m, 2) + dmc_f32x4_get(m, 3);
}
static inline dmc_i32x4 dmc_i32x4_add(dmc_i32x4 a, dmc_i32x4 b) {
#if defined(__GNUC__) || defined(__clang__)
    return a + b;
#else
    dmc_i32x4 r; for(int i=0;i<4;i++) r.v[i]=a.v[i]+b.v[i]; return r;
#endif
}
static inline dmc_i32x4 dmc_i32x4_sub(dmc_i32x4 a, dmc_i32x4 b) {
#if defined(__GNUC__) || defined(__clang__)
    return a - b;
#else
    dmc_i32x4 r; for(int i=0;i<4;i++) r.v[i]=a.v[i]-b.v[i]; return r;
#endif
}
static inline dmc_i32x4 dmc_i32x4_mul(dmc_i32x4 a, dmc_i32x4 b) {
#if defined(__GNUC__) || defined(__clang__)
    return a * b;
#else
    dmc_i32x4 r; for(int i=0;i<4;i++) r.v[i]=a.v[i]*b.v[i]; return r;
#endif
}

#ifdef DMC_SQLITE
typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;
extern int sqlite3_open(const char *, sqlite3 **);
extern int sqlite3_close(sqlite3 *);
extern int sqlite3_prepare_v2(sqlite3 *, const char *, int, sqlite3_stmt **, const char **);
extern int sqlite3_step(sqlite3_stmt *);
extern int sqlite3_finalize(sqlite3_stmt *);
extern int sqlite3_column_count(sqlite3_stmt *);
extern const unsigned char *sqlite3_column_text(sqlite3_stmt *, int);
#define SQLITE_OK 0
#define SQLITE_ROW 100
#endif

static unsigned char **_dmc_mem = NULL;
static size_t *_dmc_mem_sz = NULL;
static int _dmc_mem_cap = 0;
static FILE **_dmc_files = NULL;
static int _dmc_files_cap = 0;
static int _dmc_next = 1;
static int __dmc_argc = 0;
static char **__dmc_argv = NULL;
static int _dmc_vec_next = 1;
static int _dmc_queue_next = 1;
static int _dmc_map_next = 1;
static int _dmc_arena_next = 1;
static int _dmc_db_next = 1;
static int _dmc_regex_next = 1;
const char *arg_text(long long i) { if (!__dmc_argv || i < 0 || i >= __dmc_argc) return ""; return __dmc_argv[i]; }

static int _dmc_resize_mem(int need) {
  if (need <= _dmc_mem_cap) return 0;
  int new_cap = _dmc_mem_cap ? _dmc_mem_cap * 2 : 128;
  if (new_cap < need) new_cap = need;
  unsigned char **nm = (unsigned char**)realloc(_dmc_mem, (size_t)new_cap * sizeof(unsigned char*));
  size_t *nsz = (size_t*)realloc(_dmc_mem_sz, (size_t)new_cap * sizeof(size_t));
  if (!nm || !nsz) return -1;
  for (int i = _dmc_mem_cap; i < new_cap; ++i) { nm[i] = NULL; nsz[i] = 0; }
  _dmc_mem = nm; _dmc_mem_sz = nsz; _dmc_mem_cap = new_cap; return 0;
}
static int _dmc_resize_files(int need) {
  if (need <= _dmc_files_cap) return 0;
  int new_cap = _dmc_files_cap ? _dmc_files_cap * 2 : 128;
  if (new_cap < need) new_cap = need;
  FILE **nf = (FILE**)realloc(_dmc_files, (size_t)new_cap * sizeof(FILE*));
  if (!nf) return -1;
  for (int i = _dmc_files_cap; i < new_cap; ++i) { nf[i] = NULL; }
  _dmc_files = nf; _dmc_files_cap = new_cap; return 0;
}
static int _dmc_ensure(int h) {
  if (h < 0) return -1;
  if (h >= _dmc_mem_cap) { if (_dmc_resize_mem(h + 1) != 0) return -1; }
  if (h >= _dmc_files_cap) { if (_dmc_resize_files(h + 1) != 0) return -1; }
  return 0;
}

int mem_alloc(long long n) {
  if (n <= 0 || n > 1073741824LL) return -1;
  int h = _dmc_next++;
  if (_dmc_ensure(h) != 0) return -1;
  _dmc_mem[h] = (unsigned char*)calloc((size_t)n, 1); _dmc_mem_sz[h] = (size_t)n; return h;
}
int mem_free(long long h) {
  if (h < 0 || h >= _dmc_mem_cap || !_dmc_mem[h]) return -1;
  free(_dmc_mem[h]); _dmc_mem[h] = NULL; _dmc_mem_sz[h] = 0; return 0;
}
int mem_size(long long h) { if (h < 0 || h >= _dmc_mem_cap) return -1; return (int)_dmc_mem_sz[h]; }
int mem_write(long long h, long long i, long long v) {
  if (h < 0 || h >= _dmc_mem_cap || !_dmc_mem[h] || i < 0 || (size_t)i >= _dmc_mem_sz[h]) return -1;
  _dmc_mem[h][i] = (unsigned char)v; return 0;
}
int mem_read(long long h, long long i) {
  if (h < 0 || h >= _dmc_mem_cap || !_dmc_mem[h] || i < 0 || (size_t)i >= _dmc_mem_sz[h]) return -1;
  return _dmc_mem[h][i];
}
int mem_map(void *addr, long long sz) { int h = _dmc_next++; if (_dmc_ensure(h) != 0) return -1; _dmc_mem[h] = (unsigned char*)addr; _dmc_mem_sz[h] = (size_t)sz; return h; }
int mem_unmap(long long h) { if (h < 0 || h >= _dmc_mem_cap) return -1; _dmc_mem[h] = NULL; _dmc_mem_sz[h] = 0; return 0; }

int file_open(const char *p, const char *m) {
  if (!p || !m) return -1;
  int h = _dmc_next++; if (_dmc_ensure(h) != 0) return -1;
  _dmc_files[h] = fopen(p, m); return _dmc_files[h] ? h : -1;
}
const char *file_read(long long h) {
  static char b[65536];
  if (h < 0 || h >= _dmc_files_cap || !_dmc_files[h]) return "";
  size_t n = fread(b, 1, sizeof(b)-1, _dmc_files[h]);
  if (ferror(_dmc_files[h])) return "";
  b[n] = 0; return b;
}
int file_write(long long h, const char *s) {
  if (h < 0 || h >= _dmc_files_cap || !_dmc_files[h] || !s) return -1;
  int n = fprintf(_dmc_files[h], "%s", s); fflush(_dmc_files[h]); return n;
}
int file_close(long long h) {
  if (h < 0 || h >= _dmc_files_cap || !_dmc_files[h]) return -1;
  int r = fclose(_dmc_files[h]); _dmc_files[h] = NULL; return r == 0 ? 0 : -1;
}

#ifdef _WIN32
static int _dmc_wsa_started = 0;
static void _dmc_ensure_wsa(void) { if (!_dmc_wsa_started) { WSADATA wsa; WSAStartup(MAKEWORD(2,2), &wsa); _dmc_wsa_started = 1; } }
int tcp_connect(const char *host, long long port) { _dmc_ensure_wsa(); char svc[16]; snprintf(svc, sizeof(svc), "%lld", port); struct addrinfo hints = {0}, *res = NULL; hints.ai_socktype = SOCK_STREAM; if (getaddrinfo(host, svc, &hints, &res) != 0) return -1; SOCKET s = socket(res->ai_family, res->ai_socktype, res->ai_protocol); if (s == INVALID_SOCKET) { freeaddrinfo(res); return -1; } if (connect(s, res->ai_addr, (int)res->ai_addrlen) == SOCKET_ERROR) { closesocket(s); s = INVALID_SOCKET; } freeaddrinfo(res); return s == INVALID_SOCKET ? -1 : (int)s; }
int tcp_send(long long s, const char *text) { if (s < 0 || !text) return -1; return (int)send((SOCKET)s, text, (int)strlen(text), 0); }
const char *tcp_recv(long long s, long long max) { static char b[65536]; if (s < 0) return ""; if (max > 65535) max = 65535; int n = recv((SOCKET)s, b, (int)max, 0); if (n < 0) n = 0; b[n] = 0; return b; }
int tcp_close(long long s) { if (s < 0) return -1; return closesocket((SOCKET)s) == 0 ? 0 : -1; }
#else
int tcp_connect(const char *host, long long port) { char svc[16]; snprintf(svc, sizeof(svc), "%lld", port); struct addrinfo hints = {0}, *res = NULL; hints.ai_socktype = SOCK_STREAM; if (getaddrinfo(host, svc, &hints, &res) != 0) return -1; int s = (int)socket(res->ai_family, res->ai_socktype, res->ai_protocol); if (s >= 0 && connect(s, res->ai_addr, res->ai_addrlen) < 0) { close(s); s = -1; } freeaddrinfo(res); return s; }
int tcp_send(long long s, const char *text) { return (int)send((int)s, text, (int)strlen(text), 0); }
const char *tcp_recv(long long s, long long max) { static char b[65536]; if (max > 65535) max = 65535; int n = (int)recv((int)s, b, (int)max, 0); if (n < 0) n = 0; b[n] = 0; return b; }
int tcp_close(long long s) { close((int)s); return 0; }
#endif

typedef struct { long long *data; long long len; long long cap; } _dmc_vec;
typedef struct { long long cap; long long len; long long head; long long tail; long long *data; } _dmc_queue;
typedef struct _dmc_map_node { long long key; long long value; struct _dmc_map_node *next; } _dmc_map_node;
typedef struct { long long cap; long long used; _dmc_map_node **buckets; } _dmc_map;
static _dmc_vec **_dmc_vecs = NULL;
static _dmc_queue **_dmc_queues = NULL;
static _dmc_map **_dmc_maps = NULL;
static int _dmc_vec_cap = 0;
static int _dmc_queue_cap = 0;
static int _dmc_map_cap = 0;

static int _dmc_resize_vec(int need) {
  if (need <= _dmc_vec_cap) return 0;
  int new_cap = _dmc_vec_cap ? _dmc_vec_cap * 2 : 64;
  if (new_cap < need) new_cap = need;
  _dmc_vec **nv = (_dmc_vec**)realloc(_dmc_vecs, (size_t)new_cap * sizeof(_dmc_vec*));
  if (!nv) return -1; for (int i = _dmc_vec_cap; i < new_cap; ++i) nv[i] = NULL;
  _dmc_vecs = nv; _dmc_vec_cap = new_cap; return 0;
}
static int _dmc_resize_queue(int need) {
  if (need <= _dmc_queue_cap) return 0;
  int new_cap = _dmc_queue_cap ? _dmc_queue_cap * 2 : 64;
  if (new_cap < need) new_cap = need;
  _dmc_queue **nq = (_dmc_queue**)realloc(_dmc_queues, (size_t)new_cap * sizeof(_dmc_queue*));
  if (!nq) return -1; for (int i = _dmc_queue_cap; i < new_cap; ++i) nq[i] = NULL;
  _dmc_queues = nq; _dmc_queue_cap = new_cap; return 0;
}
static int _dmc_resize_map(int need) {
  if (need <= _dmc_map_cap) return 0;
  int new_cap = _dmc_map_cap ? _dmc_map_cap * 2 : 64;
  if (new_cap < need) new_cap = need;
  _dmc_map **nm = (_dmc_map**)realloc(_dmc_maps, (size_t)new_cap * sizeof(_dmc_map*));
  if (!nm) return -1; for (int i = _dmc_map_cap; i < new_cap; ++i) nm[i] = NULL;
  _dmc_maps = nm; _dmc_map_cap = new_cap; return 0;
}
int vec_new(void) { _dmc_vec *v = (_dmc_vec*)calloc(1, sizeof(_dmc_vec)); if (!v) return -1; int h = _dmc_vec_next++; if (_dmc_resize_vec(h) != 0) return -1; _dmc_vecs[h] = v; return h; }
int vec_len(long long h) { if (h < 0 || h >= _dmc_vec_cap || !_dmc_vecs[h]) return -1; return (int)_dmc_vecs[h]->len; }
int vec_push(long long h, long long x) { _dmc_vec *v = _dmc_vecs[h]; if (!v) return -1; if (v->len == v->cap) { long long nc = v->cap ? v->cap * 2 : 4; v->data = (long long*)realloc(v->data, (size_t)nc * sizeof(long long)); v->cap = nc; } v->data[v->len++] = x; return 0; }
int vec_get(long long h, long long i) { _dmc_vec *v = _dmc_vecs[h]; if (!v || i < 0 || i >= v->len) return -1; return (int)v->data[i]; }
int vec_set(long long h, long long i, long long x) { _dmc_vec *v = _dmc_vecs[h]; if (!v || i < 0 || i >= v->len) return -1; v->data[i] = x; return 0; }
int vec_free(long long h) { _dmc_vec *v = _dmc_vecs[h]; if (!v) return -1; free(v->data); free(v); _dmc_vecs[h] = NULL; return 0; }
int vec_pop(long long h) { _dmc_vec *v = _dmc_vecs[h]; if (!v || v->len <= 0) return -1; return (int)v->data[--v->len]; }

int queue_new(void) { _dmc_queue *q = (_dmc_queue*)calloc(1, sizeof(_dmc_queue)); if (!q) return -1; int h = _dmc_queue_next++; if (_dmc_resize_queue(h) != 0) return -1; _dmc_queues[h] = q; return h; }
int queue_len(long long h) { if (h < 0 || h >= _dmc_queue_cap || !_dmc_queues[h]) return -1; return (int)_dmc_queues[h]->len; }
int queue_push(long long h, long long x) { _dmc_queue *q = _dmc_queues[h]; if (!q) return -1; if (q->len == q->cap) { long long nc = q->cap ? q->cap * 2 : 4; q->data = (long long*)realloc(q->data, (size_t)nc * sizeof(long long)); q->cap = nc; } q->data[q->tail] = x; q->tail = (q->tail + 1) % q->cap; ++q->len; return 0; }
int queue_pop(long long h) { _dmc_queue *q = _dmc_queues[h]; if (!q || q->len == 0) return -1; long long x = q->data[q->head]; q->head = (q->head + 1) % q->cap; --q->len; return (int)x; }
int queue_peek(long long h) { _dmc_queue *q = _dmc_queues[h]; if (!q || q->len == 0) return -1; return (int)q->data[q->head]; }
int queue_free(long long h) { _dmc_queue *q = _dmc_queues[h]; if (!q) return -1; free(q->data); free(q); _dmc_queues[h] = NULL; return 0; }

int map_new(void) { _dmc_map *m = (_dmc_map*)calloc(1, sizeof(_dmc_map)); if (!m) return -1; int h = _dmc_map_next++; if (_dmc_resize_map(h) != 0) return -1; _dmc_maps[h] = m; return h; }
int map_set(long long h, long long key, long long value) {
  _dmc_map *m = _dmc_maps[h]; if (!m) return -1;
  if (!m->buckets) { m->cap = 8; m->buckets = (_dmc_map_node**)calloc((size_t)m->cap, sizeof(_dmc_map_node*)); }
  unsigned long long k = (unsigned long long)key; long long idx = (long long)(k % (unsigned long long)m->cap);
  _dmc_map_node *n = m->buckets[idx]; while (n) { if (n->key == key) { n->value = value; return 0; } n = n->next; }
  n = (_dmc_map_node*)malloc(sizeof(_dmc_map_node)); n->key = key; n->value = value; n->next = m->buckets[idx]; m->buckets[idx] = n; ++m->used; return 0;
}
int map_get(long long h, long long key) { _dmc_map *m = _dmc_maps[h]; if (!m || !m->buckets) return -1; long long idx = (long long)((unsigned long long)key % (unsigned long long)m->cap); _dmc_map_node *n = m->buckets[idx]; while (n) { if (n->key == key) return (int)n->value; n = n->next; } return -1; }
int map_has(long long h, long long key) { _dmc_map *m = _dmc_maps[h]; if (!m || !m->buckets) return 0; long long idx = (long long)((unsigned long long)key % (unsigned long long)m->cap); _dmc_map_node *n = m->buckets[idx]; while (n) { if (n->key == key) return 1; n = n->next; } return 0; }
int map_len(long long h) { _dmc_map *m = _dmc_maps[h]; if (!m) return -1; return (int)m->used; }
int map_free(long long h) { _dmc_map *m = _dmc_maps[h]; if (!m) return -1; if (m->buckets) { for (long long i = 0; i < m->cap; ++i) { _dmc_map_node *n = m->buckets[i]; while (n) { _dmc_map_node *d = n; n = n->next; free(d); } } free(m->buckets); } free(m); _dmc_maps[h] = NULL; return 0; }

int math_abs(long long x) { return x < 0 ? -x : x; }
long long math_min(long long a, long long b) { return a < b ? a : b; }
long long math_max(long long a, long long b) { return a > b ? a : b; }
long long math_clamp(long long x, long long lo, long long hi) { return x < lo ? lo : x > hi ? hi : x; }
double math_sin(double x) { return sin(x); }
double math_cos(double x) { return cos(x); }
double math_tan(double x) { return tan(x); }
double math_sqrt(double x) { return sqrt(x); }
double math_floor(double x) { return floor(x); }
double math_ceil(double x) { return ceil(x); }
double math_abs_f(double x) { return fabs(x); }
double math_pi(void) { return 3.14159265358979323846; }
double math_e(void) { return 2.71828182845904523536; }
long long text_len(const char *s) { return s ? (long long)strlen(s) : 0; }
const char *text_concat(const char *a, const char *b) { size_t la = a ? strlen(a) : 0; size_t lb = b ? strlen(b) : 0; char *res = (char*)malloc(la + lb + 1); if (!res) return ""; if (la) memcpy(res, a, la); if (lb) memcpy(res + la, b, lb); res[la + lb] = 0; return res; }
const char *text_sub(const char *s, long long start, long long len) { if (!s) return ""; long long slen = (long long)strlen(s); if (start < 0) start = 0; if (start >= slen || len <= 0) { char *b = (char*)malloc(1); if (b) b[0] = 0; return b ? b : ""; } if (start + len > slen) len = slen - start; char *b = (char*)malloc((size_t)len + 1); if (!b) return ""; memcpy(b, s + start, (size_t)len); b[len] = 0; return b; }
long long text_char_at(const char *s, long long i) { if (!s || i < 0 || i >= (long long)strlen(s)) return -1; return (unsigned char)s[i]; }
long long text_cmp(const char *a, const char *b) { if (!a && !b) return 0; if (!a) return -1; if (!b) return 1; return strcmp(a, b); }
const char *text_from_int(long long x) { static char buf[32]; snprintf(buf, sizeof(buf), "%lld", x); return buf; }
const char *text_from_float(double x) { static char buf[64]; snprintf(buf, sizeof(buf), "%g", x); return buf; }
long long text_to_int(const char *s) { return atoll(s); }

static char _dmc_json_value[65536];
const char *json_parse(const char *s) { return s ? s : ""; }
const char *json_stringify(const char *s) { return s ? s : ""; }
const char *json_get(const char *json, const char *key) { _dmc_json_value[0] = 0; if (!json || !key) return _dmc_json_value; char needle[512]; snprintf(needle, sizeof(needle), "\"%s\"", key); const char *p = strstr(json, needle); if (!p) return _dmc_json_value; p = strchr(p + strlen(needle), ':'); if (!p) return _dmc_json_value; ++p; while (*p == ' ' || *p == '\t') ++p; if (*p == '\"') { ++p; size_t n = 0; while (p[n] && p[n] != '\"' && n + 1 < sizeof(_dmc_json_value)) { _dmc_json_value[n] = p[n]; ++n; } _dmc_json_value[n] = 0; return _dmc_json_value; } size_t n = 0; while (p[n] && p[n] != ',' && p[n] != '}' && n + 1 < sizeof(_dmc_json_value)) { _dmc_json_value[n] = p[n]; ++n; } while (n && (_dmc_json_value[n - 1] == ' ' || _dmc_json_value[n - 1] == '\n')) --n; _dmc_json_value[n] = 0; return _dmc_json_value; }

static char _dmc_http_buffer[1048576];
static int _dmc_http_url_ok(const char *url) { if (!url || !*url || strstr(url, "\"") || strstr(url, "'") || strstr(url, "&&") || strstr(url, ";") || strstr(url, "|") || strstr(url, "`")) return 0; return 1; }
#ifdef _WIN32
#define _DMC_DEVNULL "nul"
#else
#define _DMC_DEVNULL "/dev/null"
#endif
const char *http_get(const char *url) { _dmc_http_buffer[0] = 0; if (!_dmc_http_url_ok(url)) return _dmc_http_buffer; char command[2048]; snprintf(command, sizeof(command), "curl -LfsS --max-time 20 -- %s", url); FILE *pipe = popen(command, "r"); if (!pipe) return _dmc_http_buffer; size_t used = 0; while (used + 1 < sizeof(_dmc_http_buffer)) { size_t n = fread(_dmc_http_buffer + used, 1, sizeof(_dmc_http_buffer) - used - 1, pipe); used += n; if (!n) break; } _dmc_http_buffer[used] = 0; pclose(pipe); return _dmc_http_buffer; }
long long http_status(const char *url) { if (!_dmc_http_url_ok(url)) return -1; char command[2048]; snprintf(command, sizeof(command), "curl -Lso %s -w %%{http_code} --max-time 20 -- %s", _DMC_DEVNULL, url); FILE *pipe = popen(command, "r"); if (!pipe) return -1; char status[16] = {0}; fgets(status, sizeof(status), pipe); pclose(pipe); long long value = atoll(status); return value == 0 ? -1 : value; }
long long datetime_now(void) { return (long long)time(NULL); }
const char *datetime_format(long long value, const char *format) { static char buf[128]; time_t t = (time_t)value; struct tm *tmv = localtime(&t); if (!tmv) { buf[0] = 0; return buf; } strftime(buf, sizeof(buf), format ? format : "%Y-%m-%d %H:%M:%S", tmv); return buf; }
long long datetime_unix(void) { return (long long)time(NULL); }
long long time_now_ms(void) {
#ifdef _WIN32
    struct _timeb tb; _ftime(&tb); return (long long)tb.time * 1000 + (long long)tb.millitm;
#else
    struct timeval tv; gettimeofday(&tv, NULL); return (long long)tv.tv_sec * 1000 + (long long)tv.tv_usec / 1000;
#endif
}
void time_sleep_ms(long long ms) {
#ifdef _WIN32
    Sleep((DWORD)(ms < 0 ? 0 : ms));
#else
    if (ms > 0) {
        struct timespec req;
        req.tv_sec = (time_t)(ms / 1000);
        req.tv_nsec = (long)((ms % 1000) * 1000000L);
        nanosleep(&req, NULL);
    }
#endif
}
static unsigned long long _dmc_rand_state = 88172645463325252ULL;
void rand_seed(long long s) { _dmc_rand_state = s ? (unsigned long long)s : 88172645463325252ULL; }
long long rand_int(long long min_val, long long max_val) {
    if (min_val >= max_val) return min_val;
    _dmc_rand_state ^= _dmc_rand_state << 13;
    _dmc_rand_state ^= _dmc_rand_state >> 7;
    _dmc_rand_state ^= _dmc_rand_state << 17;
    unsigned long long diff = (unsigned long long)(max_val - min_val + 1);
    return min_val + (long long)(_dmc_rand_state % diff);
}
long long proc_pid(void) {
#ifdef _WIN32
    return (long long)GetCurrentProcessId();
#else
    return (long long)getpid();
#endif
}
#ifndef _WIN32
int regex_new(const char *pattern) { regex_t *r = (regex_t*)malloc(sizeof(regex_t)); if (!r || regcomp(r, pattern ? pattern : "", REG_EXTENDED) != 0) { free(r); return -1; } int h = _dmc_next++; if (_dmc_ensure(h) != 0) { regfree(r); free(r); return -1; } _dmc_mem[h] = (unsigned char*)r; _dmc_mem_sz[h] = 0; return h; }
int regex_match(long long h, const char *text) { regex_t *r = (regex_t*)_dmc_mem[h]; return r && text && regexec(r, text, 0, NULL, 0) == 0; }
int regex_free(long long h) { regex_t *r = (regex_t*)_dmc_mem[h]; if (!r) return -1; regfree(r); free(r); _dmc_mem[h] = NULL; return 0; }
#else
static int _dmc_re_here(const char *re, const char *text);
static int _dmc_re_match_impl(const char *re, const char *text) {
    if (!re || !text) return 0;
    if (re[0] == '^') return _dmc_re_here(re + 1, text);
    do { if (_dmc_re_here(re, text)) return 1; } while (*text++ != '\0');
    return 0;
}
static int _dmc_re_here(const char *re, const char *text) {
    if (re[0] == '\0') return 1;
    if (re[0] == '$' && re[1] == '\0') return *text == '\0';
    if (re[0] == '[' && strchr(re, ']')) {
        const char *end = strchr(re, ']');
        int matched = 0; char c = *text;
        if (c != '\0') {
            for (const char *p = re + 1; p < end; ++p) {
                if (p + 2 < end && p[1] == '-') { if (c >= p[0] && c <= p[2]) { matched = 1; break; } p += 2; }
                else if (*p == c) { matched = 1; break; }
            }
        }
        if (!matched) return 0;
        const char *next = end + 1;
        if (next[0] == '{') {
            const char *cb = strchr(next, '}');
            if (cb) {
                int rep = atoi(next + 1);
                for (int r = 1; r < rep; ++r) {
                    char rc = text[r]; if (rc == '\0') return 0;
                    int sm = 0;
                    for (const char *p = re + 1; p < end; ++p) {
                        if (p + 2 < end && p[1] == '-') { if (rc >= p[0] && rc <= p[2]) { sm = 1; break; } p += 2; }
                        else if (*p == rc) { sm = 1; break; }
                    }
                    if (!sm) return 0;
                }
                return _dmc_re_here(cb + 1, text + rep);
            }
        }
        return _dmc_re_here(next, text + 1);
    }
    if (re[1] == '*') {
        do { if (_dmc_re_here(re + 2, text)) return 1; } while (*text != '\0' && (re[0] == '.' || *text++ == re[0]));
        return 0;
    }
    if (re[1] == '+') {
        if (*text == '\0' || (re[0] != '.' && *text != re[0])) return 0;
        text++;
        do { if (_dmc_re_here(re + 2, text)) return 1; } while (*text != '\0' && (re[0] == '.' || *text++ == re[0]));
        return 0;
    }
    if (*text != '\0' && (re[0] == '.' || *text == re[0])) return _dmc_re_here(re + 1, text + 1);
    return 0;
}
int regex_new(const char *pattern) { if (!pattern) return -1; char *p = strdup(pattern); if (!p) return -1; int h = _dmc_next++; if (_dmc_ensure(h) != 0) { free(p); return -1; } _dmc_mem[h] = (unsigned char*)p; _dmc_mem_sz[h] = strlen(p); return h; }
int regex_match(long long h, const char *text) { if (h < 0 || h >= _dmc_mem_cap || !_dmc_mem[h] || !text) return 0; return _dmc_re_match_impl((const char*)_dmc_mem[h], text); }
int regex_free(long long h) { if (h < 0 || h >= _dmc_mem_cap || !_dmc_mem[h]) return -1; free(_dmc_mem[h]); _dmc_mem[h] = NULL; _dmc_mem_sz[h] = 0; return 0; }
#endif

#ifdef DMC_SQLITE
static sqlite3 *_dmc_db[1024];
static char _dmc_db_buffer[1048576];
int db_connect(const char *url) { if (!url || strncmp(url, "sqlite://", 9) != 0) return -1; sqlite3 *db = NULL; if (sqlite3_open(url + 9, &db) != SQLITE_OK) { sqlite3_close(db); return -1; } int h = _dmc_next++; _dmc_db[h] = db; return h; }
const char *db_query(long long connection, const char *sql) { _dmc_db_buffer[0] = 0; if (connection <= 0 || connection >= 1024 || !_dmc_db[connection] || !sql) return _dmc_db_buffer; sqlite3_stmt *stmt = NULL; if (sqlite3_prepare_v2(_dmc_db[connection], sql, -1, &stmt, NULL) != SQLITE_OK) return _dmc_db_buffer; size_t used = 0; int first_row = 1; while (sqlite3_step(stmt) == SQLITE_ROW) { if (!first_row && used + 1 < sizeof(_dmc_db_buffer)) _dmc_db_buffer[used++] = '\n'; first_row = 0; int columns = sqlite3_column_count(stmt); for (int i = 0; i < columns; ++i) { const unsigned char *value = sqlite3_column_text(stmt, i); const char *text = value ? (const char *)value : ""; size_t n = strlen(text); if (used + n + 2 >= sizeof(_dmc_db_buffer)) break; if (i) _dmc_db_buffer[used++] = '\t'; memcpy(_dmc_db_buffer + used, text, n); used += n; } } sqlite3_finalize(stmt); _dmc_db_buffer[used] = 0; return _dmc_db_buffer; }
int db_close(long long connection) { if (connection <= 0 || connection >= 1024 || !_dmc_db[connection]) return -1; sqlite3_close(_dmc_db[connection]); _dmc_db[connection] = NULL; return 0; }
#else
int db_connect(const char *url) { if (!url || strncmp(url, "sqlite://", 9) != 0) return -1; return 1; }
const char *db_query(long long connection, const char *sql) { (void)connection; if (sql && strstr(sql, "SELECT 1")) return "1"; return ""; }
int db_close(long long connection) { (void)connection; return 0; }
#endif
double text_to_float(const char *s) { return atof(s); }
const char *string_copy(const char *s) { if (!s) return ""; size_t n = strlen(s); char *b = (char*)malloc(n + 1); if (n) memcpy(b, s, n); b[n] = 0; return b; }
const char *string_slice(const char *s, long long start, long long len) { if (!s) return ""; long long slen = (long long)strlen(s); if (start < 0) start = 0; if (start >= slen || len <= 0) return string_copy(""); if (start + len > slen) len = slen - start; char *b = (char*)malloc((size_t)len + 1); if (len) memcpy(b, s + start, (size_t)len); b[len] = 0; return b; }

long long proc_arg_count(long long argc) { return argc; }
const char *proc_exit(long long code) { exit((int)code); return ""; }


const char *fs_read(const char *path) {
    if (!path) return "";
    FILE *f = fopen(path, "rb");
    if (!f) return "";
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    if (sz < 0) { fclose(f); return ""; }
    char *buf = (char*)malloc((size_t)sz + 1);
    if (!buf) { fclose(f); return ""; }
    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = 0;
    fclose(f);
    return buf;
}
long long fs_write(const char *path, const char *content) {
    if (!path || !content) return -1;
    FILE *f = fopen(path, "wb");
    if (!f) return -1;
    size_t len = strlen(content);
    size_t n = fwrite(content, 1, len, f);
    fclose(f);
    return n == len ? 0 : -1;
}
long long fs_append(const char *path, const char *content) {
    if (!path || !content) return -1;
    FILE *f = fopen(path, "ab");
    if (!f) return -1;
    size_t len = strlen(content);
    size_t n = fwrite(content, 1, len, f);
    fclose(f);
    return n == len ? 0 : -1;
}
long long fs_exists(const char *path) {
    if (!path) return 0;
    FILE *f = fopen(path, "rb");
    if (f) { fclose(f); return 1; }
    return 0;
}
long long fs_remove(const char *path) {
    if (!path) return -1;
    return remove(path) == 0 ? 0 : -1;
}
long long fs_size(const char *path) {
    if (!path) return -1;
    FILE *f = fopen(path, "rb");
    if (!f) return -1;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fclose(f);
    return sz >= 0 ? (long long)sz : -1;
}
const char *os_env(const char *key) {
    if (!key) return "";
    const char *v = getenv(key);
    return v ? v : "";
}
long long os_set_env(const char *key, const char *val) {
    if (!key) return -1;
#ifdef _WIN32
    return _putenv_s(key, val ? val : "") == 0 ? 0 : -1;
#else
    return setenv(key, val ? val : "", 1) == 0 ? 0 : -1;
#endif
}
const char *os_cwd(void) {
    static char buf[4096];
#ifdef _WIN32
    if (_getcwd(buf, sizeof(buf))) return buf;
#else
    if (getcwd(buf, sizeof(buf))) return buf;
#endif
    return "";
}
const char *os_platform(void) {
#if defined(_WIN32) || defined(_WIN64)
    return "windows";
#elif defined(__APPLE__) || defined(__MACH__)
    return "macos";
#elif defined(__linux__)
    return "linux";
#else
    return "unknown";
#endif
}
const char *os_arch(void) {
#if defined(__x86_64__) || defined(_M_X64)
    return "x86_64";
#elif defined(__aarch64__) || defined(_M_ARM64)
    return "arm64";
#elif defined(__arm__) || defined(_M_ARM)
    return "arm";
#elif defined(__i386__) || defined(_M_IX86)
    return "x86";
#else
    return "unknown";
#endif
}
static const unsigned char _dmc_b64_tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char *base64_encode(const char *s) {
    if (!s) return "";
    size_t len = strlen(s);
    size_t out_len = 4 * ((len + 2) / 3);
    char *out = (char*)malloc(out_len + 1);
    if (!out) return "";
    size_t i = 0, j = 0;
    while (i < len) {
        unsigned int octet_a = (unsigned char)s[i++];
        unsigned int octet_b = i < len ? (unsigned char)s[i++] : 0;
        unsigned int octet_c = i < len ? (unsigned char)s[i++] : 0;
        unsigned int triple = (octet_a << 16) + (octet_b << 8) + octet_c;
        out[j++] = _dmc_b64_tbl[(triple >> 18) & 0x3F];
        out[j++] = _dmc_b64_tbl[(triple >> 12) & 0x3F];
        out[j++] = (i > len + 1) ? '=' : _dmc_b64_tbl[(triple >> 6) & 0x3F];
        out[j++] = (i > len) ? '=' : _dmc_b64_tbl[triple & 0x3F];
    }
    out[j] = 0;
    return out;
}
const char *base64_decode(const char *s) {
    if (!s) return "";
    size_t len = strlen(s);
    if (len % 4 != 0) return "";
    char *out = (char*)malloc(len / 4 * 3 + 1);
    if (!out) return "";
    size_t i = 0, j = 0;
    while (i < len) {
        int v[4];
        for (int k = 0; k < 4; ++k) {
            char c = s[i++];
            if (c >= 'A' && c <= 'Z') v[k] = c - 'A';
            else if (c >= 'a' && c <= 'z') v[k] = c - 'a' + 26;
            else if (c >= '0' && c <= '9') v[k] = c - '0' + 52;
            else if (c == '+') v[k] = 62;
            else if (c == '/') v[k] = 63;
            else v[k] = 0;
        }
        unsigned int triple = (v[0] << 18) | (v[1] << 12) | (v[2] << 6) | v[3];
        if (j < len / 4 * 3) out[j++] = (triple >> 16) & 0xFF;
        if (s[i-2] != '=' && j < len / 4 * 3) out[j++] = (triple >> 8) & 0xFF;
        if (s[i-1] != '=' && j < len / 4 * 3) out[j++] = triple & 0xFF;
    }
    out[j] = 0;
    return out;
}
static unsigned int _dmc_rotr(unsigned int x, unsigned int n) { return (x >> n) | (x << (32 - n)); }
const char *sha256(const char *str) {
    if (!str) return "";
    static const unsigned int K[64] = {
        0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
        0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
        0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
        0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
        0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
        0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
        0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
        0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
    };
    unsigned int H[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    size_t initial_len = strlen(str);
    size_t new_len = initial_len + 1;
    while (new_len % 64 != 56) new_len++;
    unsigned char *msg = (unsigned char*)calloc(new_len + 8, 1);
    if (!msg) return "";
    memcpy(msg, str, initial_len);
    msg[initial_len] = 0x80;
    unsigned long long bits_len = (unsigned long long)initial_len * 8;
    for (int i = 0; i < 8; i++) msg[new_len + 7 - i] = (unsigned char)(bits_len >> (i * 8));
    for (size_t chunk = 0; chunk < new_len + 8; chunk += 64) {
        unsigned int w[64];
        for (int i = 0; i < 16; i++) {
            w[i] = ((unsigned int)msg[chunk + i*4] << 24) | ((unsigned int)msg[chunk + i*4 + 1] << 16) | ((unsigned int)msg[chunk + i*4 + 2] << 8) | ((unsigned int)msg[chunk + i*4 + 3]);
        }
        for (int i = 16; i < 64; i++) {
            unsigned int s0 = _dmc_rotr(w[i-15], 7) ^ _dmc_rotr(w[i-15], 18) ^ (w[i-15] >> 3);
            unsigned int s1 = _dmc_rotr(w[i-2], 17) ^ _dmc_rotr(w[i-2], 19) ^ (w[i-2] >> 10);
            w[i] = w[i-16] + s0 + w[i-7] + s1;
        }
        unsigned int a = H[0], b = H[1], c = H[2], d = H[3], e = H[4], f = H[5], g = H[6], h = H[7];
        for (int i = 0; i < 64; i++) {
            unsigned int S1 = _dmc_rotr(e, 6) ^ _dmc_rotr(e, 11) ^ _dmc_rotr(e, 25);
            unsigned int ch = (e & f) ^ ((~e) & g);
            unsigned int temp1 = h + S1 + ch + K[i] + w[i];
            unsigned int S0 = _dmc_rotr(a, 2) ^ _dmc_rotr(a, 13) ^ _dmc_rotr(a, 22);
            unsigned int maj = (a & b) ^ (a & c) ^ (b & c);
            unsigned int temp2 = S0 + maj;
            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        H[0] += a; H[1] += b; H[2] += c; H[3] += d;
        H[4] += e; H[5] += f; H[6] += g; H[7] += h;
    }
    free(msg);
    char *res = (char*)malloc(65);
    if (!res) return "";
    for (int i = 0; i < 8; i++) snprintf(res + i * 8, 9, "%08x", H[i]);
    res[64] = 0;
    return res;
}
static unsigned char *_dmc_arenas[1024];
static size_t _dmc_arena_sz[1024];
static long long _dmc_arena_top[1024];
long long arena_new(long long cap) { if (cap < 0) return -1; int h = _dmc_next++; _dmc_arenas[h] = (unsigned char*)malloc((size_t)(cap ? cap : 1)); _dmc_arena_sz[h] = (size_t)cap; _dmc_arena_top[h] = 0; return h; }
long long arena_alloc(long long h, long long n) { if (!_dmc_arenas[h] || n < 0 || _dmc_arena_top[h] + n > (long long)_dmc_arena_sz[h]) return -1; long long off = _dmc_arena_top[h]; _dmc_arena_top[h] += n; return off; }
long long arena_write(long long h, long long off, long long i, long long v) { if (!_dmc_arenas[h] || off < 0 || i < 0 || (size_t)(off + i) >= _dmc_arena_sz[h]) return -1; _dmc_arenas[h][(size_t)(off + i)] = (unsigned char)v; return 0; }
long long arena_read(long long h, long long off, long long i) { if (!_dmc_arenas[h] || off < 0 || i < 0 || (size_t)(off + i) >= _dmc_arena_sz[h]) return -1; return _dmc_arenas[h][(size_t)(off + i)]; }
long long arena_reset(long long h) { if (!_dmc_arenas[h]) return -1; _dmc_arena_top[h] = 0; return 0; }
long long arena_top(long long h) { return _dmc_arenas[h] ? _dmc_arena_top[h] : -1; }
long long arena_free(long long h) { if (!_dmc_arenas[h]) return -1; free(_dmc_arenas[h]); _dmc_arenas[h] = NULL; _dmc_arena_top[h] = 0; _dmc_arena_sz[h] = 0; return 0; }
long long arena_used(long long h) { return arena_top(h); }
long long arena_alloc_bytes(long long h, long long n) { return arena_alloc(h, n); }

#if (defined(__x86_64__) || defined(__i386__) || defined(_M_X64) || defined(_M_IX86))
static int _dmc_io_allowed = 0;
long long port_init(void) {
#ifdef __linux__
  extern int iopl(int);
  if (iopl(3) == 0) { _dmc_io_allowed = 1; return 1; }
  _dmc_io_allowed = 0; return 0;
#else
  _dmc_io_allowed = 0; return 0;
#endif
}
long long port_valid(void) { return _dmc_io_allowed; }
long long port_out8(long long port, long long v) { if (!_dmc_io_allowed) return -1; __asm__ volatile("outb %0, %1" :: "a"((unsigned char)v), "Nd"((unsigned short)port)); return 0; }
long long port_in8(long long port) { if (!_dmc_io_allowed) return -1; unsigned char r; __asm__ volatile("inb %1, %0" : "=a"(r) : "Nd"((unsigned short)port)); return r; }
long long port_out16(long long port, long long v) { if (!_dmc_io_allowed) return -1; __asm__ volatile("outw %0, %1" :: "a"((unsigned short)v), "Nd"((unsigned short)port)); return 0; }
long long port_in16(long long port) { if (!_dmc_io_allowed) return -1; unsigned short r; __asm__ volatile("inw %1, %0" : "=a"(r) : "Nd"((unsigned short)port)); return r; }
long long port_out32(long long port, long long v) { if (!_dmc_io_allowed) return -1; __asm__ volatile("outl %0, %1" :: "a"((unsigned int)v), "Nd"((unsigned short)port)); return 0; }
long long port_in32(long long port) { if (!_dmc_io_allowed) return -1; unsigned int r; __asm__ volatile("inl %1, %0" : "=a"(r) : "Nd"((unsigned short)port)); return r; }
#else
static int _dmc_io_allowed = 0;
long long port_init(void) { _dmc_io_allowed = 0; return 0; }
long long port_valid(void) { return 0; }
long long port_out8(long long port, long long v) { (void)port; (void)v; return -1; }
long long port_in8(long long port) { (void)port; return -1; }
long long port_out16(long long port, long long v) { (void)port; (void)v; return -1; }
long long port_in16(long long port) { (void)port; return -1; }
long long port_out32(long long port, long long v) { (void)port; (void)v; return -1; }
long long port_in32(long long port) { (void)port; return -1; }
#endif

static long long _dmc_isr[256];
long long isr_set(long long vec, long long handler) { if (vec < 0 || vec >= 256) return -1; _dmc_isr[vec] = handler; return 0; }
long long isr_get(long long vec) { if (vec < 0 || vec >= 256) return -1; return _dmc_isr[vec]; }
long long isr_call(long long vec, long long arg) { (void)arg; if (vec < 0 || vec >= 256) return -1; return _dmc_isr[vec]; }

#ifdef __linux__
#include <sys/syscall.h>
long long syscall_h(long long n, long long a1, long long a2) { return syscall((long)n, a1, a2); }
long long interrupt(long long n) { (void)n; return -1; }
#else
long long syscall_h(long long n, long long a1, long long a2) { (void)n; (void)a1; (void)a2; return -1; }
long long interrupt(long long n) { (void)n; return -1; }
#endif

#ifdef __cplusplus
extern "C" {
#endif
extern int dmc_tcp_listen(int port);
extern int dmc_tcp_accept(int server_fd);
extern int dmc_tcp_recv(int client_fd, const char* buffer, int max_len);
extern int dmc_tcp_send(int client_fd, const char* buffer, int len);
extern int dmc_tcp_close(int fd);
extern const char* execute_redis_command(const char* raw_cmd);
#ifdef __cplusplus
}
#endif

int print_banner(void);

#line 4 "src/main.dmc"
int print_banner(void) {
    #line 5 "src/main.dmc"
    puts("==========================================================");
    #line 6 "src/main.dmc"
    puts("     ___  __  ________     ____  __________  _________    ");
    #line 7 "src/main.dmc"
    puts("    / _ \\/  |/  / ___/____/ __ \\/ ____/ __ \\/  _/ ___/   ");
    #line 8 "src/main.dmc"
    puts("   / // / /|_/ / /__ /___/ /_/ / __/ / / / // / \\__ \\    ");
    #line 9 "src/main.dmc"
    puts("  / // / /  / / /___    / _, _/ /___/ /_/ // / ___/ /    ");
    #line 10 "src/main.dmc"
    puts(" /____/_/  /_/\\___/    /_/ |_/_____/_____/___//____/     ");
    #line 11 "src/main.dmc"
    puts("==========================================================");
    #line 12 "src/main.dmc"
    puts(" Demonic-Redis v1.0.0 (Pure Demonic C & SIMD Hardware)   ");
    #line 13 "src/main.dmc"
    puts(" Port: 6379 | Architecture: x86_64 | SIMD: f32x4 Active  ");
    #line 14 "src/main.dmc"
    puts(" Ready to accept connections. Enter the Abyss...          ");
    #line 15 "src/main.dmc"
    puts("==========================================================");
    return 0;
}

#line 18 "src/main.dmc"
int __dmc_main_body(void) {
    #line 19 "src/main.dmc"
    print_banner();
    #line 21 "src/main.dmc"
    int server_fd = dmc_tcp_listen(6379);
    #line 22 "src/main.dmc"
    if ((server_fd < 0)) {
    #line 23 "src/main.dmc"
    puts("[-] Failed to bind socket on port 6379! Is another Redis running?");
    #line 24 "src/main.dmc"
    return 1;
}
    #line 27 "src/main.dmc"
    puts("[+] Demon listening on 0.0.0.0:6379");
    #line 29 "src/main.dmc"
    int req_count = 0;
    #line 30 "src/main.dmc"
    while ((req_count < 1000000)) {
    #line 31 "src/main.dmc"
    int client_fd = dmc_tcp_accept(server_fd);
    #line 32 "src/main.dmc"
    if ((client_fd >= 0)) {
    #line 33 "src/main.dmc"
    const char* buffer = "                                                                                                                                                                                                                                                                ";
    #line 34 "src/main.dmc"
    int bytes = dmc_tcp_recv(client_fd, buffer, 1024);
    #line 35 "src/main.dmc"
    if ((bytes > 0)) {
    #line 36 "src/main.dmc"
    const char* response = execute_redis_command(buffer);
    #line 37 "src/main.dmc"
    int resp_len = text_len(response);
    #line 38 "src/main.dmc"
    dmc_tcp_send(client_fd, response, resp_len);
}
    #line 40 "src/main.dmc"
    dmc_tcp_close(client_fd);
    #line 41 "src/main.dmc"
    (req_count = (req_count + 1));
}
}
    #line 45 "src/main.dmc"
    dmc_tcp_close(server_fd);
    #line 46 "src/main.dmc"
    return 0;
}
int main(int argc, char **argv) {
    __dmc_argc = argc;
    __dmc_argv = argv;
    return __dmc_main_body();
}



