

struct conn_state {
    struct buf buf;
    struct fhmsg_server *parent;
    struct cmd_state *cmd_state;
};

void conn_state_init(struct conn_state *conn_state, struct fhmsg_server *parent, struct cmd_state *cmd_state) {
    buf_init(&(conn_state->buf));
    conn_state->parent = parent;
    conn_state->cmd_state = cmd_state;
}