//
//  httpclient.cpp
//  libevent_ex
//
//  Created by weijiazhen on 17/6/9.
//  Copyright © 2017年 weijiazhen. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#include <event2/event.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#define MAXLINE 1024
#define DEFAULT_URL  "http://127.0.0.1:8080"

static char uri[256];
static const char *host = NULL;
static int port = 8080;
static struct event_base *main_base = NULL;
static int conn_num = 1;
static int req_num = 1;
static int conn_index = 0;

void cmd_msg_cb(int fd, short events, void *arg);

void http_request_done(struct evhttp_request *req, void *arg)
{
    char buffer[256];
    int nread;
    
    if (req == NULL) {
        printf("request failed\n");
        return;
    }
    fprintf(stderr, "Response line: %d %s\n",
            evhttp_request_get_response_code(req),req->response_code_line);
    
    while ((nread = evbuffer_remove(evhttp_request_get_input_buffer(req),
                                    buffer, sizeof(buffer)))
           > 0) {
        fwrite(buffer, nread, 1, stdout);
    }
}

int main(int argc, char **argv)
{
    const char *url = DEFAULT_URL;
    switch (argc) {
        case 2:
            conn_num = atoi(argv[1]);
            break;
        case 3:
            conn_num = atoi(argv[1]);
            req_num = atoi(argv[2]);
            break;
        case 4:
            conn_num = atoi(argv[1]);
            req_num = atoi(argv[2]);
            url = argv[3];
            break;
        default:
            break;
    }
    
    struct evhttp_uri *http_uri = NULL;
    const char *scheme, *path, *query;
    
    http_uri = evhttp_uri_parse(url);
    
    scheme = evhttp_uri_get_scheme(http_uri);
    if (scheme == NULL || (strcasecmp(scheme, "https") != 0 &&
                           strcasecmp(scheme, "http") != 0)) {
        printf("url must be http or https\n");
        return 1;
    }
    
    host = evhttp_uri_get_host(http_uri);
    port = evhttp_uri_get_port(http_uri);
    if (port == -1) {
        port = (strcasecmp(scheme, "http") == 0) ? 80 : 443;
    }
    
    path = evhttp_uri_get_path(http_uri);
    if (strlen(path) == 0) {
        path = "/";
    }
    
    query = evhttp_uri_get_query(http_uri);
    if (query == NULL) {
        snprintf(uri, sizeof(uri) - 1, "%s", path);
    } else {
        snprintf(uri, sizeof(uri) - 1, "%s?%s", path, query);
    }
    uri[sizeof(uri) - 1] = '\0';
    
    printf("url: %s\n", url);
    printf("scheme: %s\n", scheme);
    printf("host: %s\n", host);
    printf("port: %d\n", port);
    printf("path: %s\n", path);
    printf("uri: %s\n", uri);
    
    main_base = event_base_new();
    
    struct event *ev_cmd = event_new(main_base, STDIN_FILENO, EV_READ | EV_PERSIST, cmd_msg_cb,
                                     NULL);
    event_add(ev_cmd, NULL);
    
    event_base_dispatch(main_base);
    
    return 0;
}

void cmd_msg_cb(int fd, short events, void *arg) {
    char msg[MAXLINE];
    int len = read(fd, msg, MAXLINE);
    if (len <= 0) {
        perror("read error");
        exit(1);
    }
    for (int i = 0; i < conn_num; i++) {
        struct evhttp_connection *conn = evhttp_connection_base_new(main_base, NULL, host, port);
        for (int j = 0; j < req_num; j++) {
            struct evhttp_request *req = evhttp_request_new(http_request_done, main_base);
            
            evhttp_add_header(req->output_headers, "Host", host);
            
            struct evbuffer *evb = evbuffer_new();
            evbuffer_add_printf(evb, "conn_index %d req_index %d, content: %s\n",
                                conn_index + i, j,msg);
            evbuffer_add_buffer(req->output_buffer, evb);
            
            evhttp_make_request(conn, req, EVHTTP_REQ_POST, uri);
        }
    }
    conn_index += conn_num;
}
