//
//  evthread.hpp
//  libevent_ex
//
//  Created by weijiazhen on 17/6/9.
//  Copyright © 2017年 weijiazhen. All rights reserved.
//

#ifndef evthread_hpp
#define evthread_hpp

#include <pthread.h>
#include <event2/event.h>
#include <event2/event_struct.h>
#include <queue>
#include <vector>

using namespace std;

class LIBEVENT_THREAD {
public:
    LIBEVENT_THREAD();
    void start();
    void queue_push(int connfd);
    
    pthread_t thread_id;
    struct event_base *base;
    struct event notify_event;
    int notify_receive_fd;
    int notify_send_fd;
    queue<int> conn_queue;
};

class ThreadManager {
public:
    ThreadManager(int nthreads);
    void start_threads();
    void handle_conn(int connfd);
    
    vector<LIBEVENT_THREAD*> threads;
    int thread_count;
    int last_thread;
};

#endif /* evthread_hpp */
