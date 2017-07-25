#include "def.h"
#include <netinet/in.h> /* INET6_ADDRSTRLEN */
#include <stdlib.h>
#include <string.h>

#ifndef INET6_ADDRSTRLEN
#define INET6_ADDRSTRLEN 63
#endif

typedef struct
{
    uv_getaddrinfo_t getclientinfo_req;
    uv_getaddrinfo_t getaddrinfo_req;
    server_config config;
    server_ctx *servers;
    uv_loop_t *loop;
    union {
        struct sockaddr_in6 addr6;
        struct sockaddr_in addr4;
        struct sockaddr addr;
    } c_addr;
} server_state;

static void do_client_lookup(uv_getaddrinfo_t *req, int status, struct addrinfo *ai);
static void do_bind(uv_getaddrinfo_t *req, int status, struct addrinfo *ai);
static void on_connection(uv_stream_t *server, int status);

void *xmalloc(size_t size)
{
    void *ptr;

    ptr = malloc(size);
    if (ptr == NULL)
    {
        printf("out of memory, need %lu bytes", (unsigned long)size);
        exit(1);
    }

    return ptr;
}

int server_run(const server_config *cf, uv_loop_t *loop)
{
    struct addrinfo hints;
    server_state state;
    int err;

    memset(&state, 0, sizeof(state));
    state.servers = NULL;
    state.config = *cf;
    state.loop = loop;

    /* Resolve the address of the interface that we should bind to.
   * The getaddrinfo callback starts the server and everything else.
   */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    err = uv_getaddrinfo(loop,
                         &state.getclientinfo_req,
                         do_client_lookup,
                         cf->conn_host,
                         NULL,
                         &hints);
    if (err != 0)
    {
        printf("getaddrinfo: %s", uv_strerror(err));
        return err;
    }

    /* Start the event loop.  Control continues in do_bind(). */
    if (uv_run(loop, UV_RUN_DEFAULT))
    {
        abort();
    }

    /* Please Valgrind. */
    uv_loop_delete(loop);
    free(state.servers);
    return 0;
}

static void do_client_lookup(uv_getaddrinfo_t *req, int status, struct addrinfo *addrs)
{
    server_state *state;
    server_config *cf;
    uv_loop_t *loop;
    struct addrinfo hints;
    int err;
    
    state = CONTAINER_OF(req, server_state, getclientinfo_req);
    loop = state->loop;
    cf = &state->config;

    if (status < 0)
    {
        printf("unknown client host name:%s\n", cf->conn_host);
        uv_freeaddrinfo(addrs);
        uv_stop(loop);
        return;
    }

    if (addrs->ai_family == AF_INET)
    {
        state->c_addr.addr4 = *(const struct sockaddr_in *)addrs->ai_addr;
        state->c_addr.addr4.sin_port = htons(cf->conn_port);
    }
    else if (addrs->ai_family == AF_INET6)
    {
        state->c_addr.addr6 = *(const struct sockaddr_in6 *)addrs->ai_addr;
        state->c_addr.addr6.sin6_port = htons(cf->conn_port);
    }
    else
    {
        printf("cannot find this family\n");
        uv_freeaddrinfo(addrs);
        uv_stop(loop);
        return;
    }

    /* Resolve the address of the interface that we should bind to.
   * The getaddrinfo callback starts the server and everything else.
   */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    err = uv_getaddrinfo(loop,
                         &state->getaddrinfo_req,
                         do_bind,
                         cf->bind_host,
                         NULL,
                         &hints);
    if (err != 0)
    {
        printf("getaddrinfo: %s", uv_strerror(err));
        return;
    }

    uv_freeaddrinfo(addrs);
}

/* Bind a server to each address that getaddrinfo() reported. */
static void do_bind(uv_getaddrinfo_t *req, int status, struct addrinfo *addrs)
{
    char addrbuf[INET6_ADDRSTRLEN + 1];
    unsigned int ipv4_naddrs;
    unsigned int ipv6_naddrs;
    server_state *state;
    server_config *cf;
    struct addrinfo *ai;
    const void *addrv;
    const char *what;
    uv_loop_t *loop;
    server_ctx *sx;
    unsigned int n;
    int err;
    union {
        struct sockaddr addr;
        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
    } s;

    state = CONTAINER_OF(req, server_state, getaddrinfo_req);
    loop = state->loop;
    cf = &state->config;

    if (status < 0)
    {
        printf("getaddrinfo(\"%s\"): %s", cf->bind_host, uv_strerror(status));
        uv_freeaddrinfo(addrs);
        return;
    }

    ipv4_naddrs = 0;
    ipv6_naddrs = 0;
    for (ai = addrs; ai != NULL; ai = ai->ai_next)
    {
        if (ai->ai_family == AF_INET)
        {
            ipv4_naddrs += 1;
        }
        else if (ai->ai_family == AF_INET6)
        {
            ipv6_naddrs += 1;
        }
    }

    if (ipv4_naddrs == 0 && ipv6_naddrs == 0)
    {
        printf("%s has no IPv4/6 addresses", cf->bind_host);
        uv_freeaddrinfo(addrs);
        return;
    }

    state->servers =
        xmalloc((ipv4_naddrs + ipv6_naddrs) * sizeof(state->servers[0]));

    n = 0;
    for (ai = addrs; ai != NULL; ai = ai->ai_next)
    {
        if (ai->ai_family != AF_INET && ai->ai_family != AF_INET6)
        {
            continue;
        }

        if (ai->ai_family == AF_INET)
        {
            s.addr4 = *(const struct sockaddr_in *)ai->ai_addr;
            s.addr4.sin_port = htons(cf->bind_port);
            addrv = &s.addr4.sin_addr;
        }
        else if (ai->ai_family == AF_INET6)
        {
            s.addr6 = *(const struct sockaddr_in6 *)ai->ai_addr;
            s.addr6.sin6_port = htons(cf->bind_port);
            addrv = &s.addr6.sin6_addr;
        }
        else
        {
            UNREACHABLE();
        }

        if (uv_inet_ntop(s.addr.sa_family, addrv, addrbuf, sizeof(addrbuf)))
        {
            UNREACHABLE();
        }

        sx = state->servers + n;
        sx->c_addr.addr = state->c_addr.addr;
        sx->loop = loop;
        sx->idle_timeout = state->config.idle_timeout;
        CHECK(0 == uv_tcp_init(loop, &sx->tcp_handle));

        what = "uv_tcp_bind";
        err = uv_tcp_bind(&sx->tcp_handle, &s.addr, 0);
        if (err == 0)
        {

            what = "uv_listen";
            err = uv_listen((uv_stream_t *)&sx->tcp_handle, 128, on_connection);
        }

        if (err != 0)
        {
            printf("%s(\"%s:%hu\"): %s",
                   what,
                   addrbuf,
                   cf->bind_port,
                   uv_strerror(err));
            while (n > 0)
            {
                n -= 1;
                uv_close((uv_handle_t *)(state->servers + n), NULL);
            }
            break;
        }

        printf("listening on %s:%hu", addrbuf, cf->bind_port);
        n += 1;
    }

    uv_freeaddrinfo(addrs);
}

static void on_connection(uv_stream_t *server, int status)
{
    server_ctx *sx;
    client_ctx *cx;

    CHECK(status == 0);
    sx = CONTAINER_OF(server, server_ctx, tcp_handle);
    cx = xmalloc(sizeof(*cx));
    CHECK(0 == uv_tcp_init(sx->loop, &cx->incoming.handle.tcp));
    CHECK(0 == uv_accept(server, &cx->incoming.handle.stream));
    client_finish_init(sx, cx);
}
