
#define CMD_ENTRY_NAME_MAX_LEN (64)
struct cmd_entry {
    int id;
    char name[CMD_ENTRY_NAME_MAX_LEN];
    int (*send)(struct cmd_state *cmd_state, struct fhbuf *txbuf); /* actually not just send, including pre-send action, send, post-send acttion*/
    int (*recv)(struct cmd_state *cmd_state, struct fhbuf *rxbuf);
    int (*send_ack)(struct cmd_state *cmd_state, struct fhbuf *txbuf);
    int (*recv_ack)(struct cmd_state *cmd_state, struct fhbuf *rxbuf);
    int (*is_completed)(struct cmd_state *cmd_state); /* is completed on receiving? */
    int (*has_next)(struct cmd_state *cmd_state);
};