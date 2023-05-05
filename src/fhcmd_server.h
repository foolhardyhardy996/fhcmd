#pragma once 

#include "fhcmd_agent.h"

struct fhcmd_server {
    struct fhcmd_agent *parent;
    uv_loop_t *loop;
    uv_tcp_t *tcp;
    uv_thread_t *thread;
};

int fhcmd_server_open(struct fhcmd_server *server, struct fhcmd_agent *parent); 