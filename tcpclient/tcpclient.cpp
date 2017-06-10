//
//  tcpclient.cpp
//  libevent_ex
//
//  Created by weijiazhen on 17/6/9.
//  Copyright © 2017年 weijiazhen. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include <event2/event.h>
#include <event2/event_struct.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>

#define DEFAULT_HOST "127.0.0.1"
#define DEFAULT_PORT 8080
#define MAXLINE 1024

void cmd_msg_cb(int fd, short events, void *arg);
void socket_read_cb(struct bufferevent *bev, void *arg);
void event_cb(struct bufferevent *bev, short event, void *arg);

int main(int argc, char **argv)
{
    const char *host = DEFAULT_HOST;
    int port = DEFAULT_PORT;
    if (argc == 2) {
        port = atoi(argv[1]);
    } else if (argc == 3){
        host = argv[1];
        port = atoi(argv[2]);
    }
    printf("host: %s, port: %d\n", host, port);
    
    struct event_base *base = event_base_new();
    
    struct bufferevent *bev = bufferevent_socket_new(base, -1, BEV_OPT_CLOSE_ON_FREE);
    
    struct event *ev_cmd = event_new(base, STDIN_FILENO, EV_READ | EV_PERSIST, cmd_msg_cb, (void*) bev);
    event_add(ev_cmd, NULL);
    
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(port);
    inet_pton(AF_INET, host, &servaddr.sin_addr);
    
    bufferevent_socket_connect(bev, (struct sockaddr *)&servaddr, sizeof(servaddr));
    
    bufferevent_setcb(bev, socket_read_cb, NULL, event_cb, ev_cmd);
    bufferevent_enable(bev, EV_READ | EV_PERSIST);
    
    event_base_dispatch(base);
    
    return 0;
}

void cmd_msg_cb(int fd, short events, void *arg) {
    char msg[MAXLINE];
    int len = read(fd, msg, MAXLINE);
    if (len <= 0) {
        perror("read error");
        exit(1);
    }
    
    struct bufferevent *bev = (struct bufferevent*)arg;
    bufferevent_write(bev, msg, len);
}

void socket_read_cb(struct bufferevent *bev, void *arg) {
    char msg[MAXLINE];
    int len = bufferevent_read(bev, msg, MAXLINE);
    msg[len] = '\0';
    printf("msg from server: %s\n", msg);
}

void event_cb(struct bufferevent *bev, short event, void *arg) {
    if (event & BEV_EVENT_EOF) {
        printf("server closed\n");
    } else if (event & BEV_EVENT_ERROR) {
        printf("unknown error\n");
    } else if (event & BEV_EVENT_CONNECTED) {
        printf("Connect to the server successfully\n");
        return;
    }
    bufferevent_free(bev);
    struct event *ev = (struct event*)arg;
    event_free(ev);
}
