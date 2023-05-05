#pragma once 

struct cmd_ops {
    int (*send)(struct cmd_state *cmd_state, struct fhbuf *txbuf); /* actually not just send, including pre-send action, send, post-send acttion*/
    int (*recv)(struct cmd_state *cmd_state, struct fhbuf *rxbuf);
    int (*send_ack)(struct cmd_state *cmd_state, struct fhbuf *txbuf);
    int (*recv_ack)(struct cmd_state *cmd_state, struct fhbuf *rxbuf);
    int (*has_next)(struct cmd_state *cmd_state);
};