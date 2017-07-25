#include <uv.h>

#include <assert.h>

#include <stddef.h>     /* size_t, ssize_t */
#include <stdint.h>
#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/socket.h> /* sockaddr */
#include <netinet/in.h> /* sockaddr_in, sockaddr_in6 */
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static void address_cb(uv_getaddrinfo_t *req, int status, struct addrinfo *addrs)
{
    struct addrinfo *ai;
    char addrbuf[INET6_ADDRSTRLEN + 1];
    const void *addrv;
    union {
        struct sockaddr addr;
        struct sockaddr_in addr4;
        struct sockaddr_in6 addr6;
    } s;

    if (status < 0)
    {
        printf("get address info has cancel it\n");
        return;
    }
    for (ai = addrs; ai != NULL; ai = ai->ai_next)
    {
        printf("==> GET AN ADDRESS\n");
        printf("    FAMILY:%s\n",ai->ai_family == AF_INET ? "IPV4" : "IPV6");
        if (ai->ai_family == AF_INET)
        {
            s.addr4 = *(const struct sockaddr_in *)ai->ai_addr;
            addrv = &s.addr4.sin_addr;
        }
        else if (ai->ai_family == AF_INET6)
        {
            s.addr6 = *(const struct sockaddr_in6 *)ai->ai_addr;
            addrv = &s.addr6.sin6_addr;
        }
        else
        {
            printf("UNREACHABLE\n");
            return;
        }
        if (uv_inet_ntop(s.addr.sa_family, addrv, addrbuf, sizeof(addrbuf)))
        {
            printf("UNREACHABLE\n");
            return;
        }
        printf("    ADDRESS:%s\n",addrbuf);
    }
    uv_freeaddrinfo(addrs);
    uv_stop(uv_default_loop());
}

int main(int argc, char **argv)
{
    struct addrinfo hints;
    int err;
    uv_getaddrinfo_t getaddrinfo_req;

    if (argc != 2)
    {
        printf("UNKNOWN args\n");
        return 1;
    }

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    err = uv_getaddrinfo(uv_default_loop(),
                         &getaddrinfo_req,
                         address_cb,
                         argv[1],
                         NULL,
                         &hints);
    if (err != 0)
    {
        printf("getaddrinfo: %s", uv_strerror(err));
        return err;
    }

    /* Start the event loop.  Control continues in do_bind(). */
    if (uv_run(uv_default_loop(), UV_RUN_DEFAULT))
    {
        abort();
    }

    /* Please Valgrind. */
    uv_loop_delete(uv_default_loop());
    return 0;
}