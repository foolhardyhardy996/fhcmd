#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fhcmd_server.h"
#include "fhcmd_client.h"
#include "agent_config.h"
#include "fhcmd_agent.h"

struct fhcmd_agent {
    struct agent_config *config;
    int head_len;
    struct fhcmd_server *server;
    struct fhcmd_client *client;
    struct cmd_entry *cmd_registry;
    struct cmd_entry *(*pick)(struct cmd_entry *cmd_registry, uint8_t head_bytes);
};

int fhcmd_agent_init();
