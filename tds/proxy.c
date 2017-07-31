#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "def.h"

#define DEFAULT_BIND_HOST     "0.0.0.0"
#define DEFAULT_BIND_PORT     1433
#define DEFAULT_IDLE_TIMEOUT  (60 * 1000)

int main(int argc, char **argv)
{
    server_config config;
    int err;

    memset(&config, 0, sizeof(config));
    config.bind_host = DEFAULT_BIND_HOST;
    config.bind_port = DEFAULT_BIND_PORT;
    config.conn_host = "192.168.200.133";
    config.conn_port = 1433;
    config.idle_timeout = DEFAULT_IDLE_TIMEOUT;

    err = server_run(&config, uv_default_loop());
    if (err)
    {
        exit(1);
    }

    return 0;
}