

struct conn_state {
    struct fhbuf *txbuf;
    struct fhbuf *rxbuf;
    struct fhmsg_server *parent;
    struct cmd_state *cmd_state;
};

void conn_state_init(struct conn_state *conn_state, struct fhmsg_server *parent, struct cmd_state *cmd_state) {
    conn_state->txbuf = (struct fhbuf *)malloc(sizeof(struct fhbuf));
    fhbuf_init(conn_state->txbuf);
    conn_state->rxbuf = (struct fhbuf *)malloc(sizeof(struct fhbuf));
    fhbuf_init(conn_state->rxbuf);
    conn_state->parent = parent;
    conn_state->cmd_state = cmd_state;
}

void conn_state_fin(struct conn_state *conn_state) {
    free(conn_state->rxbuf);
    free(conn_state->txbuf);
}