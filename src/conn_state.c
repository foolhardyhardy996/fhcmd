

struct conn_state {
    struct fhbuf *rxbuf; // txbuf is not maintained for reuse, because it may cause misorder
    struct fhmsg_server *parent;
    struct cmd_state *cmd_state;
};

void conn_state_init(struct conn_state *conn_state, struct fhmsg_server *parent, struct cmd_state *cmd_state) {
    conn_state->rxbuf = (struct fhbuf *)malloc(sizeof(struct fhbuf));
    fhbuf_init(conn_state->rxbuf);
    conn_state->parent = parent;
    conn_state->cmd_state = cmd_state;
}

void conn_state_fin(struct conn_state *conn_state) {
    free(conn_state->rxbuf);
}