//
//  httpserver.cpp
//  libevent_ex
//
//  Created by weijiazhen on 17/6/9.
//  Copyright © 2017年 weijiazhen. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <event2/keyvalq_struct.h>
#include <event2/http.h>
#include <event2/event.h>
#include <event2/buffer.h>

#define DEFAULT_PORT 8080

void http_request_cb(struct evhttp_request *req, void *arg);
void print_request(struct evhttp_request *req, void *arg);
void php_request_cb(struct evhttp_request *req, void *arg);

static char cbuf[256];

int main(int argc, char **argv)
{
    int port = DEFAULT_PORT;
    if (argc == 2) {
        port = atoi(argv[1]);
    }
    
    struct event_base *base = event_base_new();
    
    struct evhttp *http = evhttp_new(base);
    evhttp_set_gencb(http, http_request_cb, NULL);
    evhttp_set_cb(http, "/index.php", php_request_cb, NULL);
    evhttp_bind_socket(http, "0.0.0.0", port);
    
    printf("http server is running...\n");
    event_base_dispatch(base);
    
    return 0;
}

void php_request_cb(struct evhttp_request *req, void *arg) {
    printf("run php_request_cb...\n");
}

void http_request_cb(struct evhttp_request *req, void *arg) {
    
    print_request(req, arg);
    
    struct evbuffer *evb = evbuffer_new();
    evbuffer_add_printf(evb, "response from luohan server\n");
    evbuffer_add(evb, cbuf, sizeof(cbuf));
    evhttp_send_reply(req, HTTP_OK, "OK", evb);

    if (evb) {
        evbuffer_free(evb);
    }
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
        memset(cbuf, 0, sizeof(cbuf));
        n = evbuffer_remove(buf, cbuf, sizeof(cbuf));
        if (n > 0)
            (void) fwrite(cbuf, 1, n, stdout);
    }
    puts(">>>");
}
