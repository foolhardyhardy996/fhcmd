
#define CMD_ENTRY_NAME_MAX_LEN (64)
struct cmd_entry {
    int id;
    char name[CMD_ENTRY_NAME_MAX_LEN];
    int (*send)(struct cmd_state *cmd_state); /* actually not just send, including pre-send action, send, post-send acttion*/
    int (*recv)(void *cmd_state, struct fhbuf *buf);
    int (*send_ack)(void *cmd_struct);
    int (*recv_ack)(void *cmd_struct, struct fhbuf *buf);
    int (*is_completed)(void *cmd_struct);
};