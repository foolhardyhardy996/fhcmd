#include "fhcmd_agent.h"

static void run_event_loop(void *arg);
static void receive_incomming_connection(uv_stream_t *tcp, int status);
static void alloc_from_rxbuf(uv_handle_t *conn, size_t size, uv_buf_t *buf);

int fhcmd_server_open(struct fhcmd_server *server, struct fhcmd_agent *parent) {
    struct sockaddr_in sockaddr;
    int ret;

    server->parent = parent;
    server->loop = (uv_loop_t *)malloc(sizeof(uv_loop_t)); // release it when closing
    ret = uv_loop_init(server->loop); // close it when closing the server
    if (ret  != 0) {
        printf("[ERROR]: fail to init event loop: %s\n", uv_strerror(ret));
        goto release_loop;
    }
    server->tcp = (uv_tcp_t *)malloc(sizeof(uv_tcp_t)); // release it when closing the server
    ret = uv_tcp_init(server->loop, server->tcp);
    if (ret != 0) {
        printf("[ERROR]: fail to init tcp server: %s\n", uv_strerror(ret));
        goto release_tcp;
    }
    server->tcp->data = (void *)server;
    ret = uv_ip4_addr("localhost", parent->config->port, &sockaddr);
    if (ret != 0) {
        printf("[ERROR]: fail to construct socket address for %s and %d\n", "localhost", parent->config->port);
        goto release_tcp;
    }
    ret = uv_tcp_bind(server->tcp, (const struct sockaddr *)&sockaddr, 0);
    if (ret != 0) {
        printf("[ERROR]: fail to bind socket address to tcp server: %s\n", uv_strerror(ret));
        goto release_tcp;
    }
    ret = uv_listen((uv_stream_t *)server->tcp, agent->config->backlog, receive_incomming_connection);
    if (ret != 0) {
        printf("[ERROR]: fail to listen: %s\n", uv_strerror(ret));
        goto release_tcp;
    server->thread = (uv_thread_t *)malloc(sizeof(uv_thread_t)); // release when closing the server
    ret = uv_thread_create(server->thread, run_event_loop, (void *)server); // join when closing the server
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
}

static void receive_incomming_connection(uv_stream_t *tcp, int status) {
    struct fhcmd_server *server = (fhmsg_server *)tcp->data;
    uv_tcp_t *conn;
    struct conn_state *conn_state;
    struct cmd_state *cmd_state;
    int ret;

    if (status != 0) {
        printf("[ERROR]: error on receiving connection: %s\n", uv_strerror(status));
        return;
    }
    conn = (uv_tcp_t *)malloc(sizeof(uv_tcp_t)); // release it when terminating or aborting connection
    ret = uv_tcp_init(server->loop, conn);
    if (ret != 0) {
        printf("[ERROR]: fail to initialize connection: %s\n", uv_strerror(ret));
        goto release_conn;
    }
    cmd_state = (struct cmd_state *)malloc(sizeof(struct cmd_state));
    conn_state = (struct conn_state *)malloc(sizeof(struct conn_state)); // release it when terminating or aborting connection
    cmd_state_init(cmd_state, conn_state); // where to finalize it?
    conn_state_init(conn_state, server, conn_state); // where to release it?
    conn->data = (void *)conn_state;
    ret = uv_accept(tcp, (uv_stream_t *)conn);
    if (ret != 0) {
        printf("[ERROR]: fail to accept connection: %s\n", uv_strerror(ret));
        goto release_conn_state;
    }
    ret = uv_read_start((uv_stream_t *)conn, alloc_from_rxbuf, process_avaliable_data);
    if (ret != 0) {
        printf("[ERROR]: fail to start reading: %s\n", uv_strerror(ret));
        goto release_conn_state;
    }

release_conn_state:
    conn_state_fin(conn_state);
    free(conn_state);
release_cmd_state:
    cmd_state_fin(cmd_state);
    free(cmd_state);
release_conn:
    free(conn);
}

static void alloc_from_rxbuf(uv_handle_t *conn, size_t size, uv_buf_t *buf) {
    struct conn_state *conn_state = (struct conn_state *)conn->data;
    if (size > FHBUF_CAP - conn_state->rxbuf.len) {
        rxbuf->base = NULL;
        rxbuf->len = 0;
        return;
    }
    if (size > FHBUF_CAP - conn_state->rxbuf.len - conn_state->rxbuf.head) {
        fhbuf_compact(&(conn_state->rxbuf));
    }
    buf->base = conn_state->rxbuf.base + conn_state->rxbuf.len; // update rxbuf later when the read actually happen
    buf->len = size;
}

static void abort_conn(uv_stream_t *conn) {
    uv_close((uv_handle_t *)conn, /* free_aborted_conn */)
}

static void free_aborted_conn(uv_handle_t *conn) {
    struct conn_state *conn_state = (struct conn_state *)conn;
    conn_state_fin(conn_state);
    free(conn_state);
    free(conn);
}

static void process_avaliable_data(uv_stream_t *conn, ssize_t nread, const uv_buf_t *buf) {
    struct conn_state *conn_state;
    struct cmd_entry *cmd_entry;
    struct cmd_state *cmd_state, *next_cmd_state;
    struct ack_state *ack_state;
    uv_write_t *wt;
    uv_buf_t *buf;
    int ret;

    if (buf->base == NULL) {
        // rxbuf is full, abort
        printf("[ERROR]: rxbuf is full, connection abort\n");
        abort_conn(conn);
        return;
    }
    conn_state = (struct conn_state *)conn->data;
    cmd_state = conn_state->cmd_state;
    cmd_entry = cmd_state->cmd_entry;
    if (nread < 0) { 
        if (nread == UV_EOF && cmd_entry->is_complete(cmd_state)) {
            return;
        } else {
            printf("[ERROR]: read error: %s\n", uv_strerror(nread));
            abort_conn(conn);
        }
    } else if (nread == 0) {
        return;
    } else {
        /* update conn_state->buf */
        conn_state->buf.len += nread;
        /* handle the received data progressively */
        ret = cmd_entry->recv(cmd_state, &(conn_state->rxbuf));
        if (ret != 0 && ret != CMD_COMPLETE) {
            printf("[ERROR]: recv return %ld(0x%lx)\n", ret, ret);
            abort_conn(conn);
            return;
        }
        if (ret == FHCMD_COMPLETE) { 
            /* send ack (possibly in chain) */
            // cmd_state should be passed to wt and maintained by wt then
            cmd_ack_state = (struct cmd_ack_state *)malloc(sizeof(cmd_ack_state)); // where to release it?
            cmd_ack_state_init(cmd_ack_state, cmd_state);
            next_cmd_state = (struct cmd_state *)malloc(sizeof(struct cmd_state)); // where to free it?
            cmd_state_init(next_cmd_state, conn_state); // where to finalize it?
            conn_state->cmd_state = next_cmd_state;
            try_launch_write_request(conn);
            /* spawn a cmd_ack_state */
            /* detach current cmd_state */
            if (cmd_entry->has_next(cmd_state) == 0) {
                /* stop further read */
                uv_read_stop(conn);
            }
        }
    }
}

struct wt_state {
    uv_write_t *wt;
    struct fhbuf *txbuf;
    uv_buf_t *buf;
};

static void send_ack(struct ack_state *ack_state) {
    int ret;
    struct cmd_state *cmd_state = ack_state->parent;
    struct conn_state *conn_state = cmd_state->parent;
    struct fhbuf *txbuf;
    uv_write_t 

    ret = 
}

static void try_launch_write_request(uv_stream_t *conn) {
    ret = cmd_entry->send_ack(cmd_state, &(conn_state->txbuf)); // update txbuf in write callback
    if (ret == 0) { // have something to write
        launch_write_request(conn);
    } else if (ret != FHCMD_COMPLETE) { // other error condition
        printf("[ERROR]: send_ack return %ld(0x%lx)\n", ret, ret);
        abort_conn(conn);
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

void notify_after_closing(uv_handle_t *server) {
    printf("[INFO]: tcp server closed.\n");
}

int fhcmd_server_close(struct fhcmd_server *server) {
    uv_close((uv_handle_t *)server->tcp, notify_after_closing);
    uv_thread_join(server->thread); // okay to free resource
    uv_loop_close(server->loop);
    free(server->thread);
    free(server->tcp);
    free(server->loop);
    server->parent = NULL;
    return 0;
}