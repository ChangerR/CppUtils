#ifndef __PROXY_DEF_H
#define __PROXY_DEF_H
#include <uv.h>

#include <assert.h>
#include <netinet/in.h> /* sockaddr_in, sockaddr_in6 */
#include <stddef.h>     /* size_t, ssize_t */
#include <stdint.h>
#include <sys/socket.h> /* sockaddr */

struct client_ctx;

typedef struct
{
    const char *bind_host;
    unsigned short bind_port;
    const char *conn_host;
    unsigned short conn_port;
    unsigned int idle_timeout;
} server_config;

typedef struct
{
    unsigned int idle_timeout; /* Connection idle timeout in ms. */
    uv_tcp_t tcp_handle;
    uv_loop_t *loop;
    union {
        struct sockaddr_in6 addr6;
        struct sockaddr_in addr4;
        struct sockaddr addr;
    } c_addr;
} server_ctx;

typedef struct
{
    unsigned char rdstate;
    unsigned char wrstate;
    unsigned int idle_timeout;
    struct client_ctx *client; /* Backlink to owning client context. */
    ssize_t result;
    union {
        uv_handle_t handle;
        uv_stream_t stream;
        uv_tcp_t tcp;
        uv_udp_t udp;
    } handle;
    uv_timer_t timer_handle; /* For detecting timeouts. */
    uv_write_t write_req;
    /* We only need one of these at a time so make them share memory. */
    union {
        uv_getaddrinfo_t addrinfo_req;
        uv_connect_t connect_req;
        uv_req_t req;
        struct sockaddr_in6 addr6;
        struct sockaddr_in addr4;
        struct sockaddr addr;
        char buf[2048]; /* Scratch space. Used to read data into. */
    } t;
} conn;

typedef struct client_ctx
{
    unsigned int state;
    server_ctx *sx; /* Backlink to owning server context. */
    conn incoming;  /* Connection with the SOCKS client. */
    conn outgoing;  /* Connection with upstream. */
} client_ctx;

/* server.c */
int server_run(const server_config *cf, uv_loop_t *loop);

/* client.c */
void client_finish_init(server_ctx *sx, client_ctx *cx);

void *xmalloc(size_t size);

#define UNREACHABLE() CHECK(!"Unreachable code reached.")

/* ASSERT() is for debug checks, CHECK() for run-time sanity checks.
 * DEBUG_CHECKS is for expensive debug checks that we only want to
 * enable in debug builds but still want type-checked by the compiler
 * in release builds.
 */
#if defined(NDEBUG)
#define ASSERT(exp)
#define CHECK(exp)   \
    do               \
    {                \
        if (!(exp))  \
            abort(); \
    } while (0)
#define DEBUG_CHECKS (0)
#else
#define ASSERT(exp) assert(exp)
#define CHECK(exp) assert(exp)
#define DEBUG_CHECKS (1)
#endif

/* This macro looks complicated but it's not: it calculates the address
 * of the embedding struct through the address of the embedded struct.
 * In other words, if struct A embeds struct B, then we can obtain
 * the address of A by taking the address of B and subtracting the
 * field offset of B in A.
 */
#define CONTAINER_OF(ptr, type, field) \
    ((type *)((char *)(ptr) - ((char *)&((type *)0)->field)))

#endif