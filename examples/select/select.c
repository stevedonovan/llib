#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <llib/list.h>

#include "select.h"

struct SelectTimer_;

struct Select_ {
    fd_set fdset;
    int nfds;
    List *fds;  // we are waiting on these for reading
    bool error;
    // timer stuff
    int tfdw;
    int tfdr;
    List *timers; // SelectTimer *
    // reading from stdin
    SelectReadProc stdin_commands;
    // once-off millisecond timer
    struct SelectTimer_ *milli_timer;
    struct timeval *p_timeval;
};

typedef unsigned char byte;

struct SelectTimer_ {
    SelectTimerProc callback;
    void *data;
    byte id;
    int pid;
    Select *s;
};

// will spawn a sleep/write process for each new timer
static pid_t start_timer_thread(int secs, int twfd, byte id)
{
    pid_t pid = fork();
    if (pid == 0) { // child
        while (1) {
            sleep(secs);
           int n = write(twfd,&id,1);
            if (n != 1) {
                fprintf(stderr,"I really could not write 1 byte: %d\n",n);
            }
        }
    } // otherwise parent    
    return pid;
}

static void Select_dispose(Select *s) {
    obj_unref(s->fds);
    if (s->p_timeval)  //??
        obj_unref(s->p_timeval);
    if (s->timers) {
        // this stops the timers from removing themselves from the list!
        List *timers = s->timers;
        s->timers = NULL;
        obj_unref(timers);
        close(s->tfdw);
        close(s->tfdr);
    }
}

Select *select_new() {
    Select *me = obj_new(Select,Select_dispose);
    me->nfds = 0;
    me->tfdw = -1;
    me->fds = list_new_ptr();
    me->timers = NULL;
    me->p_timeval = NULL;
    return me;
}

#define list_addp(ls,i) list_add(ls,(void*)(i))

void select_add_read(Select *s, int fd) {
    list_addp(s->fds,fd);
    if (fd >= s->nfds)
        s->nfds = fd + 1;
}

int select_open(Select *s, const char *str, int flags) {
    int oflags = 0;
    if (flags & SelectReadWrite)
        oflags = O_RDWR;
    else if (flags & SelectRead)
        oflags = O_RDONLY;
    else if (flags & SelectWrite)
        oflags = O_WRONLY;
    if (flags & SelectNonBlock)
         oflags |= O_NONBLOCK;

    int fd = open(str,oflags);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    select_add_read(s, fd);
    return fd;        
}

List *select_read_fds(Select *s) {
    return s->fds;
}

bool select_remove_read(Select *s, int fd) {
    ListIter iter = list_find(s->fds,(void*)fd);
    if (! iter) 
        return false;
    list_delete (s->fds,iter);
    int mfd = 0;
    FOR_LIST(item,s->fds) {
        int fd = (int)item->data;
        if (fd > mfd)
            mfd = fd;
    }
    s->nfds = mfd + 1;
    return true;
}

bool select_can_read(Select *s, int fd) {
    return FD_ISSET(fd,&s->fdset);
}

void select_add_reader(Select *s, SelectReadProc reader) {
    select_add_read(s,STDIN_FILENO);
    s->stdin_commands = reader;
}

static void SelectTimer_dispose(SelectTimer *st) {
    // this is the crucial thing!
    kill(st->pid,9);
    // the rest is bookkeeping
    List *timers = st->s->timers;
    if (! timers) // because the select state is shutting down!
        return; 
    ListIter iter = list_find(timers,st);
    if (! iter) {
        printf("cannot find the timer in list?\n");
    } else {
        list_delete(timers,iter);
    }        
}

void select_timer_kill(SelectTimer *st) {
    obj_unref(st);
}

/// fire a timer `callback` every `secs` seconds, passing it `data`.
SelectTimer *select_add_timer(Select *s, int secs, SelectTimerProc callback, void *data) {
    if (s->tfdw == -1) { // timers need a pipe!
        int pipefd[2];
        int res = pipe(pipefd);
        s->tfdr = pipefd[0];
        s->tfdw = pipefd[1];
         // and we will read from this
        select_add_read(s,s->tfdr);        
        if (res == -1) {
            perror("timer setup");
            return NULL;
        }
        s->timers = list_new_ref();
    }
           
    SelectTimer *st = obj_new(SelectTimer,SelectTimer_dispose);
    st->s = s;
    st->callback = callback;
    st->data = data;
    st->id = list_size(s->timers);
    st->pid = start_timer_thread(secs,s->tfdw,st->id);    
    list_addp(s->timers,st);
    return st;
}

bool select_do_later(Select *s, int msecs, SelectTimerProc callback, void *data) {
    if (s->milli_timer != NULL) // we're already waiting...
        return false;
    SelectTimer *st = obj_new(SelectTimer,NULL);
    st->s = s;
    s->milli_timer = st;
    st->callback = callback;
    st->data = data;
    s->p_timeval = obj_new(struct timeval,NULL);
    s->p_timeval->tv_sec = 0; // no seconds
    s->p_timeval->tv_usec =  1000*msecs;  // microseconds    
    return true;
}

static void handle_timeout(Select *s) {
    s->milli_timer->callback(s->milli_timer->data);
    obj_unref(s->milli_timer);
    obj_unref(s->p_timeval);
    s->p_timeval = NULL;
    s->milli_timer = NULL;
}

#define LINE_SIZE 256

int select_select(Select *s) {
    fd_set *fds = &s->fdset;
    s->error = false;
    
top:
    
    FD_ZERO(fds);
    FOR_LIST(item,s->fds) {
        FD_SET((int)item->data,fds);
    }
    
    int res = select(s->nfds,fds,NULL,NULL,s->p_timeval);
    if (res < 0) {
        s->error = true;
        return -1;
    }
    
    if (res == 0) { // timeout
        handle_timeout(s);
        goto top;
    }
    
    if (s->stdin_commands && select_can_read(s,STDIN_FILENO)) {
        char buff[LINE_SIZE];
        int len = read(STDIN_FILENO,buff,LINE_SIZE);
        if (len == 0) { // ctrl+D
            return -1;
        }
        buff[len-1] = '\0'; // strip \n
        if (s->stdin_commands(buff) != 0)
            return -1;  // user wants out!
        goto top;
    }
    
    if (s->timers && list_size(s->timers) > 0 && select_can_read(s,s->tfdr)) {
        byte id;
        SelectTimer *finis = NULL;
        int n = read(s->tfdr,&id,1);
        if (n != 1) {
            printf("timer: expecting 1, got %d bytes\n",n);
        }
        FOR_LIST(item,s->timers) {
            SelectTimer *st = (SelectTimer*)item->data;
            if (id == st->id) {
                if (! st->callback(st->data))
                    finis = st;
            }
        }
        if (finis)
            select_timer_kill(finis);
        goto top;
    }
    return res;
}


