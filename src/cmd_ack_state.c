
struct cmd_ack_state {
    struct cmd_state *parent;
};

void cmd_ack_state_init(struct cmd_ack_state *cmd_ack_state, struct cmd_state *parent) {
    cmd_ack_state->parent = parent;
}