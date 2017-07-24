#include <stdio.h>
#include <uv.h>

static void my_alloc_cb(uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
    buf->base = malloc(suggested_size);
    buf->len = suggested_size;
}

void on_new_connection(uv_stream_t* server,int status)
{
    if (status == -1)
    {
        return;
    }

    uv_tcp_t* client = (uv_tcp_t*) malloc(sizeof(uv_tcp_t));
    uv_tcp_init(uv_default_loop,client);
    if (uv_accept(server,(uv_stream_t*)client) == 0)
    {
        uv_read_start((uv_stream_t*)client,my_alloc_cb,client_read);
    }
    else
    {
        uv_close((uv_handle_t*) client,NULL);
    }
}

int main(int argc, char **argv)
{
    uv_tcp_t server;
    struct sockaddr_in addr;
    uv_ip4_addr("0.0.0.0", 6000, &addr);
    uv_tcp_init(uv_default_loop, &server);
    uv_tcp_bind(&server,&addr,0);
    if (uv_listen((uv_stream_t*)&server,128,on_new_connection))
    {
        printf("listen port 6000 failed\n");
        return 1;
    }
    return uv_run(loop, UV_RUN_DEFAULT);
}