

struct fhmsg_server {
    uv_loop_t *loop;
    uv_tcp_t *tcp;
    uv_thread_t *thread;
};

int fhmsg_server_init(struct fhmsg_server *server, struct fhmsg_agent *agent) {
    struct sockaddr_in sockaddr;
    int ret;

    server->loop = (uv_loop_t *)malloc(sizeof(uv_loop_t)); // where to release it?
    uv_loop_init(server->loop); // where to closse it?
    server->tcp = (uv_tcp_t *)malloc(sizeof(uv_tcp_t)); // where to release it?
    ret = uv_tcp_init(server->loop, server->tcp);
    if (ret != 0) {
        printf("[ERROR]: fail to init tcp server: %s\n", uv_strerror(ret));
        goto release_tcp;
    }
    server->tcp->data = (void *)server;
    ret = uv_ip4_addr("localhost", agent->server_config->port, &sockaddr);
    if (ret != 0) {
        printf("[ERROR]: fail to construct socket address for %s and %d\n", "localhost", agent->server_config->port);
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
        goto release_tcp;
    }
    return 0;
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

static void on_read(uv_stream_t *conn, ssize_t nread, const uv_buf_t *buf) {
    struct conn_state *conn_state;
    struct cmd_entry *cmd_entry;
    struct cmd_state *cmd_state;
    uv_write_t *wt;
    uv_shutdown_t *sd;
    int ret;

    if (buf->base == NULL) {

    }
    if (nread < 0) { 
        if (/* the cmd is not complete*/) {

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
        cmd_entry->recv();
        if (/* cmd is complete */) {
            /* send ack */
            if (/* there is more cmd */) {

            } else {
                /* shutdown */
            }
        }
    }
}