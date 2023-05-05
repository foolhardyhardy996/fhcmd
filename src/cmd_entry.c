
#define CMD_ENTRY_NAME_MAX_LEN (64)
struct cmd_entry {
    int id;
    char name[CMD_ENTRY_NAME_MAX_LEN];
    // behavior pattern:
    // done something -> return 0
    // nothing to do -> return some errcode 
    // err -> return err
    int (*send)(struct cmd_state *cmd_state, struct fhbuf *txbuf); /* actually not just send, including pre-send action, send, post-send acttion*/
    int (*recv)(struct cmd_state *cmd_state, struct fhbuf *rxbuf);
    int (*send_ack)(struct cmd_state *cmd_state, struct fhbuf *txbuf);
    int (*recv_ack)(struct cmd_state *cmd_state, struct fhbuf *rxbuf);
    int (*has_next)(struct cmd_state *cmd_state);
};