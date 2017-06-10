//
//  evthread.cpp
//  libevent_ex
//
//  Created by weijiazhen on 17/6/9.
//  Copyright © 2017年 weijiazhen. All rights reserved.
//

#include "evthread.hpp"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define MAXLINE 1024

static int init_count = 0;
static pthread_mutex_t init_lock;
static pthread_cond_t init_cond;

static void thread_notify_process(int fd, short which, void *arg);
static void *worker_libevent(void *arg);
static void event_handler(const int fd, const short which, void *arg);

struct conn {
    int fd;
    struct event event;
    LIBEVENT_THREAD *thread;
};


LIBEVENT_THREAD::LIBEVENT_THREAD()
{
    int fds[2];
    if (pipe(fds)) {
        perror("Can't create notify pipe");
        exit(1);
    }
    
    notify_receive_fd = fds[0];
    notify_send_fd = fds[1];
    
    base = event_base_new();
    if (!base) {
        fprintf(stderr, "Can't allocate event base\n");
        exit(1);
    }
    
    event_assign(&notify_event, base, notify_receive_fd, EV_READ | EV_PERSIST,
                 thread_notify_process, this);
    if (event_add(&notify_event, 0) == -1) {
        fprintf(stderr, "Can't monitor libevent notify pipe\n");
        exit(1);
    }
}

void LIBEVENT_THREAD::start()
{
    pthread_attr_t  attr;
    int             ret;
    
    pthread_attr_init(&attr);
    
    if ((ret = pthread_create(&thread_id, &attr, worker_libevent, this)) != 0) {
        fprintf(stderr, "Can't create thread: %s\n",
                strerror(ret));
        exit(1);
    }
}

void LIBEVENT_THREAD::queue_push(int connfd)
{
    printf("pushing %d to thread %ld\n", connfd, thread_id);
    conn_queue.push(connfd);
    
    char buf[1] = {'c'};
    if (write(notify_send_fd, buf, 1) != 1) {
        perror("Writing to thread notify pipe");
    }
}

ThreadManager::ThreadManager(int nthreads):thread_count(nthreads)
{
    last_thread = -1;
    pthread_mutex_init(&init_lock, NULL);
    pthread_cond_init(&init_cond, NULL);
    for (int i = 0; i < thread_count; i++) {
        LIBEVENT_THREAD *thread = new LIBEVENT_THREAD();
        threads.push_back(thread);
    }
    start_threads();
}

void ThreadManager::start_threads()
{
    printf("starting threads...\n");
    for (int i = 0; i < thread_count; i++) {
        threads[i]->start();
    }
    
    /* Wait for all the threads to set themselves up before returning. */
    pthread_mutex_lock(&init_lock);
    while (init_count < thread_count) {
        pthread_cond_wait(&init_cond, &init_lock);
    }
    pthread_mutex_unlock(&init_lock);
}

void ThreadManager::handle_conn(int connfd)
{
    int tid = (last_thread + 1) % thread_count;
    threads[tid]->queue_push(connfd);
}

static void *worker_libevent(void *arg)
{
    pthread_mutex_lock(&init_lock);
    init_count++;
    pthread_cond_signal(&init_cond);
    pthread_mutex_unlock(&init_lock);
    
    LIBEVENT_THREAD *thread = (LIBEVENT_THREAD*)arg;
    printf("thread_id: %ld, event_base:%p\n", thread->thread_id, &(*(thread->base)));
    event_base_loop(thread->base, 0);
    
    return NULL;
}

static void thread_notify_process(int fd, short which, void *arg)
{
    LIBEVENT_THREAD *thread = (LIBEVENT_THREAD*)arg;
    
    char buf[1];
    if (read(fd, buf, 1) != 1) {
        fprintf(stderr, "Can't read from libevent pipe\n");
        return;
    }
    
    int connfd = thread->conn_queue.front();
    thread->conn_queue.pop();
    conn *c = (conn*)calloc(1, sizeof(conn));
    if (c == NULL) {
        fprintf(stderr, "Failed to allocate conn object\n");
        return;
    }
    c->fd = connfd;
    c->thread = thread;
    event_assign(&c->event, thread->base, c->fd, EV_READ | EV_PERSIST, event_handler, c);
    event_add(&c->event, NULL);
}

static void event_handler(const int fd, const short which, void *arg)
{
    conn *c = (conn*)arg;
    
    printf("thread %ld is handling fd: %d\n", c->thread->thread_id, fd);
    
    char msg[MAXLINE];
    int len = read(fd, msg, MAXLINE);
    if (len > 0) {
        msg[len] = '\0';
        printf("msg from client %d: %s\n", fd, msg);
        char reply_msg[MAXLINE] = "server has got your msg";
        write(fd, reply_msg, strlen(reply_msg));
    } else {
        if (len == 0) {
            printf("client %d closed\n", fd);
        } else {
            perror("read error");
        }
        
        event_free(&c->event);
        close(c->fd);
        free(c);
    }
}
