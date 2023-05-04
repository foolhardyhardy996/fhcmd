#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

struct comm_state {
    uint8_t *head;
    int head_len;
    uint8_t *payload;
    int payload_len;
    uint8_t *ackhead;
    int ackhead_len;
    uint8_t *ackpayload;
    int ackpayload_len;
};

struct fhmsg_agent {
    /*something like config?*/ server_config;
    int head_len;
    struct fhmsg_server server;
    struct fhmsg_client client;
    struct cmd_entry *cmd_registry;

};

