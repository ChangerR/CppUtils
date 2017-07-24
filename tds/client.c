#include "def.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>

/* A connection is modeled as an abstraction on top of two simple state
 * machines, one for reading and one for writing.  Either state machine
 * is, when active, in one of three states: busy, done or stop; the fourth
 * and final state, dead, is an end state and only relevant when shutting
 * down the connection.  A short overview:
 *
 *                          busy                  done           stop
 *  ----------|---------------------------|--------------------|------|
 *  readable  | waiting for incoming data | have incoming data | idle |
 *  writable  | busy writing out data     | completed write    | idle |
 *
 * We could remove the done state from the writable state machine. For our
 * purposes, it's functionally equivalent to the stop state.
 *
 * When the connection with upstream has been established, the client_ctx
 * moves into a state where incoming data from the client is sent upstream
 * and vice versa, incoming data from upstream is sent to the client.  In
 * other words, we're just piping data back and forth.  See conn_cycle()
 * for details.
 *
 * An interesting deviation from libuv's I/O model is that reads are discrete
 * rather than continuous events.  In layman's terms, when a read operation
 * completes, the connection stops reading until further notice.
 *
 * The rationale for this approach is that we have to wait until the data
 * has been sent out again before we can reuse the read buffer.
 *
 * It also pleasingly unifies with the request model that libuv uses for
 * writes and everything else; libuv may switch to a request model for
 * reads in the future.
 */
enum conn_state
{
    c_busy, /* Busy; waiting for incoming data or for a write to complete. */
    c_done, /* Done; read incoming data or write finished. */
    c_stop, /* Stopped. */
    c_dead
};

/* Session states. */
enum sess_state
{
    s_req_start,     /* Start waiting for request data. */
    s_req_parse,     /* Wait for request data. */
    s_req_lookup,    /* Wait for upstream hostname DNS lookup to complete. */
    s_req_connect,   /* Wait for uv_tcp_connect() to complete. */
    s_proxy_start,   /* Connected. Start piping data. */
    s_proxy,         /* Connected. Pipe data back and forth. */
    s_kill,          /* Tear down session. */
    s_almost_dead_0, /* Waiting for finalizers to complete. */
    s_almost_dead_1, /* Waiting for finalizers to complete. */
    s_almost_dead_2, /* Waiting for finalizers to complete. */
    s_almost_dead_3, /* Waiting for finalizers to complete. */
    s_almost_dead_4, /* Waiting for finalizers to complete. */
    s_dead           /* Dead. Safe to free now. */
};

static int do_proxy_start(client_ctx *cx);
static int do_proxy(client_ctx *cx);
static int do_kill(client_ctx *cx);
static int do_almost_dead(client_ctx *cx);
static void do_next(client_ctx *cx);

static void conn_read(conn *c);
static void conn_read_done(uv_stream_t *handle,
                           ssize_t nread,
                           const uv_buf_t *buf);
static void conn_alloc(uv_handle_t *handle, size_t size, uv_buf_t *buf);
static void conn_write(conn *c, const void *data, unsigned int len);
static void conn_write_done(uv_write_t *req, int status);
static void conn_close(conn *c);
static void conn_close_done(uv_handle_t *handle);

/* |incoming| has been initialized by server.c when this is called. */
void client_finish_init(server_ctx *sx, client_ctx *cx)
{
    conn *incoming;
    conn *outgoing;

    cx->sx = sx;
    cx->state = s_req_start;
    s5_init(&cx->parser);

    incoming = &cx->incoming;
    incoming->client = cx;
    incoming->result = 0;
    incoming->rdstate = c_stop;
    incoming->wrstate = c_stop;
    incoming->idle_timeout = sx->idle_timeout;
    CHECK(0 == uv_timer_init(sx->loop, &incoming->timer_handle));

    outgoing = &cx->outgoing;
    outgoing->client = cx;
    outgoing->result = 0;
    outgoing->rdstate = c_stop;
    outgoing->wrstate = c_stop;
    outgoing->idle_timeout = sx->idle_timeout;
    CHECK(0 == uv_tcp_init(cx->sx->loop, &outgoing->handle.tcp));
    CHECK(0 == uv_timer_init(cx->sx->loop, &outgoing->timer_handle));

    /* Wait for the initial packet. */
    conn_read(incoming);
}

/* This is the core state machine that drives the client <-> upstream proxy.
 * We move through the initial handshake and authentication steps first and
 * end up (if all goes well) in the proxy state where we're just proxying
 * data between the client and upstream.
 */
static void do_next(client_ctx *cx)
{
    int new_state;

    ASSERT(cx->state != s_dead);
    switch (cx->state)
    {
    case s_proxy_start:
        new_state = do_proxy_start(cx);
        break;
    case s_proxy:
        new_state = do_proxy(cx);
        break;
    case s_kill:
        new_state = do_kill(cx);
        break;
    case s_almost_dead_0:
    case s_almost_dead_1:
    case s_almost_dead_2:
    case s_almost_dead_3:
    case s_almost_dead_4:
        new_state = do_almost_dead(cx);
        break;
    default:
        UNREACHABLE();
    }
    cx->state = new_state;

    if (cx->state == s_dead)
    {
        if (DEBUG_CHECKS)
        {
            memset(cx, -1, sizeof(*cx));
        }
        free(cx);
    }
}

/* Assumes that cx->outgoing.t.sa contains a valid AF_INET/AF_INET6 address. */
static int do_req_start(client_ctx *cx)
{
    conn *incoming;
    conn *outgoing;
    int err;

    incoming = &cx->incoming;
    outgoing = &cx->outgoing;
    ASSERT(incoming->rdstate == c_stop);
    ASSERT(incoming->wrstate == c_stop);
    ASSERT(outgoing->rdstate == c_stop);
    ASSERT(outgoing->wrstate == c_stop);

    err = conn_connect(outgoing);
    if (err != 0)
    {
        printf("connect error: %s\n", uv_strerror(err));
        return do_kill(cx);
    }

    return s_proxy_start;
}

static void conn_read(conn *c)
{
    ASSERT(c->rdstate == c_stop);
    CHECK(0 == uv_read_start(&c->handle.stream, conn_alloc, conn_read_done));
    c->rdstate = c_busy;
    conn_timer_reset(c);
}

static void conn_read_done(uv_stream_t *handle,
                           ssize_t nread,
                           const uv_buf_t *buf)
{
    conn *c;

    c = CONTAINER_OF(handle, conn, handle);
    ASSERT(c->t.buf == buf->base);
    ASSERT(c->rdstate == c_busy);
    c->rdstate = c_done;
    c->result = nread;

    uv_read_stop(&c->handle.stream);
    do_next(c->client);
}

static void conn_alloc(uv_handle_t *handle, size_t size, uv_buf_t *buf)
{
    conn *c;

    c = CONTAINER_OF(handle, conn, handle);
    ASSERT(c->rdstate == c_busy);
    buf->base = c->t.buf;
    buf->len = sizeof(c->t.buf);
}

/* Assumes that c->t.sa contains a valid AF_INET or AF_INET6 address. */
static int conn_connect(conn *c)
{
    ASSERT(c->t.addr.sa_family == AF_INET ||
           c->t.addr.sa_family == AF_INET6);
    conn_timer_reset(c);
    return uv_tcp_connect(&c->t.connect_req,
                          &c->handle.tcp,
                          &c->t.addr,
                          conn_connect_done);
}

static void conn_connect_done(uv_connect_t *req, int status)
{
    conn *c;

    if (status == UV_ECANCELED)
    {
        return; /* Handle has been closed. */
    }

    c = CONTAINER_OF(req, conn, t.connect_req);
    c->result = status;
    do_next(c->client);
}