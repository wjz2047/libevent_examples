//
//  httpproxy.cpp
//  libevent_ex
//
//  Created by weijiazhen on 17/6/9.
//  Copyright © 2017年 weijiazhen. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <event2/keyvalq_struct.h>
#include <event2/http.h>
#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http_struct.h>
#include <string.h>

#define DEFAULT_PORT 8080
#define TARGET_HOST "180.76.187.193"
#define TARGET_PORT 8080

static int default_port = DEFAULT_PORT;
static const char *target_host = TARGET_HOST;
static int target_port = TARGET_PORT;

void http_request_cb(struct evhttp_request *req, void *arg);
void print_request(struct evhttp_request *req, void *arg);
void php_request_cb(struct evhttp_request *req, void *arg);

static struct event_base *main_base = NULL;

void http_request_done(struct evhttp_request *req, void *arg)
{
    struct evhttp_request *origin_req = (struct evhttp_request*)arg;
    
    struct evkeyvalq *headers = evhttp_request_get_input_headers(req);
    struct evkeyval *header;
    for (header = headers->tqh_first; header;
         header = header->next.tqe_next) {
        evhttp_add_header(origin_req->output_headers, header->key, header->value);
    }
    
    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    evhttp_send_reply(origin_req, req->response_code, req->response_code_line, buf);
    
    printf("proxy done\n");
}

int main(int argc, char **argv)
{
    
    switch (argc) {
        case 2:
            default_port = atoi(argv[1]);
            break;
        case 3:
            default_port = atoi(argv[1]);
            target_host = argv[2];
            break;
        case 4:
            default_port = atoi(argv[1]);
            target_host = argv[2];
            target_port = atoi(argv[3]);
            break;
        default:
            break;
    }
    
    main_base = event_base_new();
    
    struct evhttp *http = evhttp_new(main_base);
    evhttp_set_gencb(http, http_request_cb, NULL);
    evhttp_set_cb(http, "/index.php", php_request_cb, NULL);
    evhttp_bind_socket(http, "0.0.0.0", default_port);
    
    printf("http server is running...\n");
    event_base_dispatch(main_base);
    
    return 0;
}

void php_request_cb(struct evhttp_request *req, void *arg) {
    printf("run php_request_cb...\n");
}

void http_request_cb(struct evhttp_request *req, void *arg)
{
    struct evhttp_connection *conn = evhttp_connection_base_new(main_base, NULL,
                                                                target_host,
                                                                target_port);
    if (conn == NULL) {
        printf("Failed to create connection\n");
        return;
    }
    struct evhttp_request *new_req = evhttp_request_new(http_request_done, req);
    
    evhttp_add_header(new_req->output_headers, "Host", target_host);
    evbuffer_add_buffer(new_req->output_buffer, req->input_buffer);
    
    printf("making a new request...\n");
    evhttp_make_request(conn, new_req, EVHTTP_REQ_POST, "/");
}

void print_request(struct evhttp_request *req, void *arg) {
    const char *cmdtype;
    struct evkeyvalq *headers;
    struct evkeyval *header;
    struct evbuffer *buf;
    
    switch (evhttp_request_get_command(req)) {
        case EVHTTP_REQ_GET: cmdtype = "GET"; break;
        case EVHTTP_REQ_POST: cmdtype = "POST"; break;
        case EVHTTP_REQ_HEAD: cmdtype = "HEAD"; break;
        case EVHTTP_REQ_PUT: cmdtype = "PUT"; break;
        case EVHTTP_REQ_DELETE: cmdtype = "DELETE"; break;
        case EVHTTP_REQ_OPTIONS: cmdtype = "OPTIONS"; break;
        case EVHTTP_REQ_TRACE: cmdtype = "TRACE"; break;
        case EVHTTP_REQ_CONNECT: cmdtype = "CONNECT"; break;
        case EVHTTP_REQ_PATCH: cmdtype = "PATCH"; break;
        default: cmdtype = "unknown"; break;
    }
    
    printf("Received a %s request for %s\nHeaders:\n",
           cmdtype, evhttp_request_get_uri(req));
    
    headers = evhttp_request_get_input_headers(req);
    for (header = headers->tqh_first; header;
         header = header->next.tqe_next) {
        printf("  %s: %s\n", header->key, header->value);
    }
    
    buf = evhttp_request_get_input_buffer(req);
    puts("Input data: <<<");
    if (evbuffer_get_length(buf)) {
        int n;
        char cbuf[128];
        n = evbuffer_copyout(buf, cbuf, sizeof(cbuf));
        if (n > 0)
            (void) fwrite(cbuf, 1, n, stdout);
    }
    puts(">>>");
}
