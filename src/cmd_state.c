

struct cmd_state {
    size_t head_bytes_len;
    size_t body_bytes_len;
    size_t tail_bytes_len;
    size_t ack_head_bytes_len;
    size_t ack_body_bytes_len;
    size_t ack_tail_bytes_len;
    void *cmd_struct;
    struct cmd_entry *cmd_entry; 
    struct conn_state *parent;
};

void cmd_state_init(struct cmd_state *cmd_state, struct ) {

}