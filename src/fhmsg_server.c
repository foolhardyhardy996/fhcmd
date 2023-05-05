

struct fhcmd_server {
    struct fhcmd_agent *parent;
    uv_loop_t *loop;
    uv_tcp_t *tcp;
    uv_thread_t *thread;
};
// how to properly stop server is tricky.
// First, request to close tcp
// Then, every connection from the tcp will be cancelled
//     handle those cancelled connections
// When the loop ends
//     close the loop

int fhcmd_server_open(struct fhcmd_server *server, struct fhcmd_agent *parent) {
    struct sockaddr_in sockaddr;
    int ret;

    server->parent = agent;
    server->loop = (uv_loop_t *)malloc(sizeof(uv_loop_t)); // where to release it?
    uv_loop_init(server->loop); // where to closse it?
    server->tcp = (uv_tcp_t *)malloc(sizeof(uv_tcp_t)); // where to release it?
    ret = uv_tcp_init(server->loop, server->tcp);
    if (ret != 0) {
        printf("[ERROR]: fail to init tcp server: %s\n", uv_strerror(ret));
        goto release_tcp;
    }
    server->tcp->data = (void *)server;
    ret = uv_ip4_addr("localhost", parent->server_config->port, &sockaddr);
    if (ret != 0) {
        printf("[ERROR]: fail to construct socket address for %s and %d\n", "localhost", parent->server_config->port);
        goto release_tcp;
    }
    ret = uv_tcp_bind(server->tcp, (const struct sockaddr *)&sockaddr, 0);
    if (ret != 0) {
        printf("[ERROR]: fail to bind socket address to tcp server: %s\n", uv_strerror(ret));
        goto release_tcp;
    }
    ret = uv_listen((uv_stream_t *)server->tcp, agent->server_config->backlog, /*on_connection callback*/);
    if (ret != 0) {
        printf("[ERROR]: fail to listen: %s\n", uv_strerror(ret));
        goto release_tcp;
    server->thread = (uv_thread_t *)malloc(sizeof(uv_thread_t)); // where to release it?
    ret = uv_thread_create(server->thread, /*thread callback*/, /*callback arg*/); // where to join?
    if (ret != 0) {
        printf("[ERROR]: fail to create thread: %s\n", uv_strerror(ret));
        goto release_thread;
    }
    return 0;

release_thread:
    free(server->thread);
release_tcp:
    free(server->tcp);
release_loop:
    uv_loop_close(server->loop);
    free(server->loop);
    return -1;
}

static void run_event_loop(void *arg) {
    struct fhcmd_server *server = (struct fhcmd_server *)arg;
    int ret;

    ret = uv_run(server->loop, UV_RUN_DEFAULT); // when will it return? and what should I do when return?
    if (ret != 0) {
        printf("[ERROR]: uv_run error: %s\n" uv_strerror(ret));
    }
    // what kind finalize should I do here?
    uv_loop_close(server->loop);
}

static void on_connection(uv_stream_t *tcp, int status) {
    struct fhmsg_server *server = (fhmsg_server *)tcp->data;
    uv_tcp_t *conn;
    struct conn_state *conn_state;
    int ret;

    if (status != 0) {
        printf("[ERROR]: error status on connection: %s\n", uv_strerror(status));
        return;
    }
    conn = (uv_tcp_t *)malloc(sizeof(uv_tcp_t)); // where to release it?
    ret = uv_tcp_init(server->loop, conn);
    if (ret != 0) {
        printf("[ERROR]: fail to initialize connection: %s\n", uv_strerror(ret));
        goto release_conn;
    }
    conn_state = (struct conn_state *)malloc(sizeof(struct conn_state)); //where to release it?
    conn_state_init(conn_state, server);
    conn->data = (void *)conn_state;
    ret = uv_accept(tcp, (uv_stream_t *)conn);
    if (ret != 0) {
        printf("[ERROR]: fail to accept connection: %s\n", uv_strerror(ret));
        goto release_conn_state;
    }
    ret = uv_read_start((uv_stream_t *)conn, /*alloc*/, /*on_read*/);
    if (ret != 0) {
        printf("[ERROR]: fail to start reading: %s\n", uv_strerror(ret));
        goto release_conn_state;
    }

release_conn_state:
    conn_state_fin(conn_state);
    free(conn_state);
release_conn:
    free(conn);
}

static void alloc_from_conn_state_buf(uv_handle_t *conn, size_t size, uv_buf_t *buf) {
    struct conn_state *conn_state = (struct conn_state *)conn->data;
    if (size > BUF_CAP - conn_state->buf.len) {
        buf->base = NULL;
        buf->len = 0;
        return;
    }
    if (size > BUF_CAP - conn_state->buf.len - conn_state->buf.head) {
        buf_compact(&(conn_state->buf));
    }
    buf->base = conn_state->buf.base + conn_state->buf.len; // update buf later when the read actually happen
    buf->len = size;
}

static void abort_conn(uv_stream_t *conn) {

}

static void on_read(uv_stream_t *conn, ssize_t nread, const uv_buf_t *buf) {
    struct conn_state *conn_state;
    struct cmd_entry *cmd_entry;
    struct cmd_state *cmd_state;
    uv_write_t *wt;
    uv_buf_t *buf;
    uv_shutdown_t *sd;
    int ret;

    if (buf->base == NULL) {
        /* err ack and abort */
    }
    if (nread < 0) { 
        if (/* the cmd is not complete*/) {
            /* err ack and abort */
        } else {
            if (nread == U_EOF) {

            } else {

            }
        }
    } else if (nread == 0) {

    } else {
        /* update conn_state->buf */
        conn_state = (struct conn_state *)conn->data;
        conn_state->buf.len += nread;
        /* handle the received data progressively */
        cmd_state = conn_state->cmd_state;
        cmd_entry = cmd_state->cmd_entry;
        cmd_entry->recv(cmd_state, &(conn_state->rxbuf));
        if (cmd_entry->is_completed(cmd_state)) {
            /* send ack (possibly in chain) */
            try_launch_write_request(conn);
            if (cmd_entry->has_next(cmd_state)) {
                /* clear all the states for processing next cmd */
            } else {
                /* shutdown */
            }
        }
    }
}

static void try_launch_write_request(uv_stream_t *conn) {
    ret = cmd_entry->send_ack(cmd_state, &(conn_state->txbuf)); // update txbuf in write callback
    if (ret == 0) { // have something to write
        launch_write_request(conn);
    } else { // nothing to write or other error condition

    }
}

static void launch_write_request(uv_stream_t *conn) {
    uv_write_t *wt;
    uv_buf_t *buf;
    int ret;

    wt = (uv_write_t *)malloc(sizeof(uv_write_t)); // where to release it?
    buf = (uv_buf_t *)malloc(sizeof(uv_buf_t)); // where to release it?
    wt->data = (void *)buf;
    buf->base = conn_state->txbuf->base + conn_state->txbuf->head;
    buf->len = conn_state->txbuf->len;
    ret = uv_write(wt, conn, buf, 1, /* after write callback */);
    /* write error can't be elegantly handled */
    if (ret != 0) {
        /* shutdown and abort */
    }
}

void write_ack(uv_write_t *wt, int status) {
    uv_stream_t *conn = (uv_stream_t *)wt->handle;
    struct conn_state *conn_state = (struct conn_state *)conn->data;
    struct cmd_state *cmd_state = conn_state->cmd_state;
    struct cmd_entry *cmd_entry = cmd_state->cmd_entry;
    uv_write_t *wt;
    uv_buf_t *buf;

    if (status != 0) {

    } else {
        /* write success, so update txbuf */
        conn_state->txbuf->head = 0;
        conn_state->txbuf->len = 0;
        /* launch further write, if any */
        try_launch_write_request(conn);
    }
}

int fhcmd_server_close(struct fhcmd_server *server) {
    uv_close((uv_handle_t *)server->tcp, /* after close tcp, wait the event loop ends */);
}