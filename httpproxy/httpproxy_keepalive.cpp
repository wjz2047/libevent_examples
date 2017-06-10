//
//  httpproxy_keepalive.cpp
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
#define TARGET_PORT 8080
#define DSP_COUNT 2
#define DEFAULT_URL  "http://127.0.0.1:8080"

static char uri[256];
static const char* host = NULL;
static int default_port = DEFAULT_PORT;
static int target_port = TARGET_PORT;
static struct event_base *main_base = NULL;
static struct evhttp_connection **conns;
static int dsp_count = DSP_COUNT;

static char hosts[3][64] = {"180.76.187.193","13.115.248.81", "123.56.76.137"};

void http_request_cb(struct evhttp_request *req, void *arg);
void print_request(struct evhttp_request *req, void *arg);
void php_request_cb(struct evhttp_request *req, void *arg);

void http_request_done(struct evhttp_request *req, void *arg)
{
    struct evhttp_request *origin_req = (struct evhttp_request*)arg;
    
    struct evkeyvalq *headers = evhttp_request_get_input_headers(req);
    struct evkeyval *header;
    for (header = headers->tqh_first; header; header = header->next.tqe_next) {
        evhttp_add_header(origin_req->output_headers, header->key, header->value);
    }
    struct evbuffer *buf = evhttp_request_get_input_buffer(req);
    // WARNNING: can't send reply to origin_req twice!!!
    evhttp_send_reply(origin_req, req->response_code, req->response_code_line, buf);
    
    printf("proxy done\n");
}

int main(int argc, char **argv)
{
    switch (argc) {
        case 2:
            dsp_count = atoi(argv[1]);
            break;
        case 3:
            dsp_count = atoi(argv[1]);
            default_port = atoi(argv[2]);
            break;
        case 4:
            dsp_count = atoi(argv[1]);
            default_port = atoi(argv[2]);
            target_port = atoi(argv[3]);
            break;
        default:
            break;
    }
    
    main_base = event_base_new();
    conns = (struct evhttp_connection **)calloc(dsp_count,
                                                sizeof(struct evhttp_connection *));
    if (conns == NULL) {
        fprintf(stderr, "Failed to allocate connection structures\n");
        exit(1);
    }
    for (int i = 0; i < dsp_count; i++) {
        conns[i] = evhttp_connection_base_new(main_base, NULL, hosts[i], target_port);
        if (conns[i] == NULL) {
            printf("Failed to create connection\n");
            exit(1);
        }
    }
    
    struct evhttp *http = evhttp_new(main_base);
    evhttp_set_gencb(http, http_request_cb, NULL);
    evhttp_set_cb(http, "/index.php", php_request_cb, NULL);
    evhttp_bind_socket(http, "0.0.0.0", default_port);
    
    event_base_dispatch(main_base);
    
    return 0;
}

void php_request_cb(struct evhttp_request *req, void *arg) {
    printf("run php_request_cb...\n");
}

void http_request_cb(struct evhttp_request *req, void *arg) {
    int datalen = 0;
    if ((datalen = evbuffer_get_length(req->input_buffer)) <= 0) {
        printf("no data in req\n");
        return;
    }
    
    char *data = new char[datalen];
    evbuffer_remove(req->input_buffer, data, datalen);
    
    for (int i = 0; i < dsp_count; i++) {
        struct evhttp_request *new_req = evhttp_request_new(http_request_done, req);
        
        evhttp_add_header(new_req->output_headers, "Host", hosts[i]);
        evhttp_add_header(new_req->output_headers, "Connection", "keep-alive");
        evbuffer_add(new_req->output_buffer, data, datalen);
        
        evhttp_make_request(conns[i], new_req, EVHTTP_REQ_POST, "/");
    }
    
    delete [] data;
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
