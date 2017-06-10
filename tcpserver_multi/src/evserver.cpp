//
//  evserver.cpp
//  libevent_ex
//
//  Created by weijiazhen on 17/6/9.
//  Copyright © 2017年 weijiazhen. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <event2/listener.h>

#include "evthread.hpp"

#define DEFAULT_PORT 8080

#define LISTENQ 10

#define THREAD_NUM 10

void listener_cb(struct evconnlistener *listener, int fd, struct sockaddr *sock,
                 int socklen, void *arg);

static ThreadManager *thread_manager = NULL;

int main(int argc, char **argv)
{
    // start work threads
    thread_manager = new ThreadManager(THREAD_NUM);
    
    int port = DEFAULT_PORT;
    if (argc == 2) {
        port = atoi(argv[1]);
    }
    
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);
    
    struct event_base *base = event_base_new();
    
    evconnlistener *listener = evconnlistener_new_bind(base, listener_cb, base,
                                                       LEV_OPT_REUSEABLE | LEV_OPT_CLOSE_ON_FREE,
                                                       LISTENQ, (struct sockaddr*) &servaddr,
                                                       sizeof(servaddr));
    
    printf("tcp server is running on port %d ...\n", port);
    event_base_dispatch(base);
    
    event_base_free(base);
    return 0;
}

void listener_cb(struct evconnlistener *listener, int fd, struct sockaddr *sock,
                 int socklen, void *arg) {
    printf("accept a client: %d\n", fd);
    thread_manager->handle_conn(fd);
}
