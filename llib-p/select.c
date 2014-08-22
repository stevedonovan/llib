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

typedef unsigned char byte;

struct SelectChan_ {
    int twrite;
    int tread;
};

struct SelectTimer_;

struct Select_ {
    fd_set fdset_read;
    fd_set fdset_write;
    int nfds;
    List *fds;  // (SelectFile) we are waiting on these for reading
    bool error;
    // timer support
    SelectChan *tchan;
    List *timers; //(SelectTimer)
    // once-off millisecond timer
    struct SelectTimer_ *milli_timer;
    struct timeval *p_timeval;
    bool once_off;
};

struct SelectTimer_ {
    SelectTimerProc callback;
    void *data;
    SelectChan *tchan;
    byte id;
    int secs;
    int pid;
    Select *s;
};

struct SelectFile_ {
    int fd;    
    int flags;
    const char *name;
    SelectReadProc handler;
    void *handler_data;
    bool exit_on_close;
    bool writing;
};

int select_thread(SelectTimerProc callback, void *data) {
    pid_t pid = fork();
    if (pid == 0) { // child
        callback(data);
        _exit(0); // child must die!
    } // parent...
    return pid;
}

void select_sleep(int msec) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 1000*msec;
    select(0,NULL,NULL,NULL,&tv);
}

static void timer_thread(SelectTimer *st) {
    while (1) {
        sleep(st->secs);
        chan_write(st->tchan,&st->id,1);
    }
}

static SelectChan *timer_chan(Select *s) {
    return s->tchan;
}

static void Select_dispose(Select *s) {
    obj_unref(s->fds);
    if (s->timers) {
        obj_unref(timer_chan(s));
        // this stops the timers from removing themselves from the list!
        List *timers = s->timers;
        s->timers = NULL;
        obj_unref(timers);
    }
}

Select *select_new() {
    Select *me = obj_new(Select,Select_dispose);
    me->nfds = 0;
    me->fds = list_new_ref();
    me->timers = NULL;
    me->p_timeval = NULL;
    me->milli_timer = NULL;
    return me;
}

void SelectFile_dispose(SelectFile *sf) {
    if (sf->name)
        obj_unref(sf->name);
    //printf("disposing %d\n",sf->fd);
}

#define SFILE(item) ((SelectFile*)((item)->data))

SelectFile *select_add(Select *s, int fd, bool writing) {
    SelectFile *sf;
    FOR_LIST_T(SelectFile*,sf,s->fds) {
        if (sf->fd == fd)
            return sf;
    }

    sf = obj_new(SelectFile,SelectFile_dispose);
    sf->fd = fd;
    sf->name = NULL;
    sf->handler = NULL;
    sf->handler_data = NULL;
    sf->exit_on_close = false;
    sf->writing = writing;
    list_add(s->fds,sf);
    if (fd >= s->nfds)
        s->nfds = fd + 1;
    return sf;
}

SelectFile *select_add_read(Select *s, int fd) {
    return select_add(s,fd, false);
}

SelectFile *select_add_read_chan(Select *s, SelectChan *chan) {
    return select_add_read(s,chan->tread);
}

SelectFile *select_add_write(Select *s, int fd) {
    return select_add(s, fd, true);
}

SelectFile *select_add_write_chan(Select *s, SelectChan *chan) {
    return select_add_write(s,chan->twrite);
}

static int select_open_file(const char *str, int flags) {
    int oflags = 0;
    if (flags & SelectReadWrite)
        oflags = O_RDWR;
    else if (flags & SelectRead)
        oflags = O_RDONLY;
    else if (flags & SelectWrite)
        oflags = O_WRONLY;
    if (flags & SelectNonBlock)
         oflags |= O_NONBLOCK;

    return open(str,oflags);
}

int select_open(Select *s, const char *str, int flags) {
    int fd = select_open_file(str,flags);
    if (fd == -1) {
        perror("open");
        return -1;
    }
    SelectFile *sf = select_add_read(s, fd);
    sf->name = str_ref(str);
    sf->flags = flags;
    return fd;        
}

int *select_read_fds(Select *s) {
    int *res = array_new(int,list_size(s->fds)), i = 0;
    FOR_LIST(item,s->fds) {
        res[i++] = SFILE(item)->fd;
    }
    return res;
}

#define FOR_LIST_NEXT(var,list) for (ListIter var = (list)->first, _next=var->_next; \
var != NULL; var=_next,_next=var ? var->_next : NULL)

bool select_remove_read(Select *s, int fd) {
    int mfd = 0;
    FOR_LIST_NEXT(item,s->fds) {
        SelectFile *sf = SFILE(item);
        if (sf->fd == fd) {
            (list_delete(s->fds,item)); // obj_unref
        } else 
        if (fd > mfd) {
            mfd = fd;
        }
    }
    s->nfds = mfd + 1;
    return true;
}

bool select_can_read(Select *s, int fd) {
    return FD_ISSET(fd,&s->fdset_read);
}

bool select_can_write(Select *s, int fd) {
    return FD_ISSET(fd,&s->fdset_write);
}

void select_add_reader(Select *s, int fd, bool close, SelectReadProc reader, void *data) {
    if (fd == 0)
        fd = STDIN_FILENO;
    SelectFile *sf = select_add_read(s,fd);
    sf->handler = reader;
    sf->handler_data = data;
    sf->exit_on_close = close;
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
        list_remove(timers,iter);
    }        
}

void select_timer_kill(SelectTimer *st) {
    obj_unref(st);
}

static void Chan_dispose(SelectChan *chan) {
    if (chan->tread != -1)
        close(chan->tread);
    if (chan->twrite != -1)
        close(chan->twrite);
}

SelectChan *chan_new() {
     int pipefd[2];
     int res = pipe(pipefd);
    if (res != 0)
        return NULL;
    SelectChan *chan = obj_new(SelectChan,Chan_dispose);
    chan->tread = pipefd[0];
    chan->twrite = pipefd[1];
    return chan;    
}

int chan_write(SelectChan *chan, void *buff, int sz) {
    int n = write(chan->twrite,buff,sz);
    if (sz != n) {
        fprintf(stderr,"wanted to write %d, wrote %d\n",sz,n);
    }
    return n;
}

int chan_read(SelectChan *chan, void *buff, int sz) {
    return read(chan->tread,buff,sz);
}

int chan_close_read(SelectChan *chan) {
    close(chan->tread);
    chan->tread = -1;
    return chan->twrite;
}

int chan_close_write(SelectChan *chan) {
    close(chan->twrite);
    chan->twrite = -1;
    return chan->tread;
}

/// fire a timer `callback` every `secs` seconds, passing it `data`.
SelectTimer *select_add_timer(Select *s, int secs, SelectTimerProc callback, void *data) {
    SelectChan *chan;
    if (s->timers == NULL) { 
        chan = chan_new(); // timers need a channel
        if (chan == NULL) {
            perror("timer setup");
            return NULL;
        }
         // and we will watch the read end...
        select_add_read(s,chan->tread);        
        s->timers = list_new_ref();
        s->tchan = chan;
    } else { // timers share the same channel
        chan = timer_chan(s);
    }
           
    SelectTimer *st = obj_new(SelectTimer,SelectTimer_dispose);
    st->s = s;
    st->callback = callback;
    st->data = data;
    st->secs = secs;
    st->tchan = chan;
    st->id = list_size(s->timers);
    st->pid = select_thread((SelectTimerProc)timer_thread,st);
    list_add(s->timers,st);
    return st;
}

void select_set_timeout(Select *s, int msecs) {
    if (! s->p_timeval)
        s->p_timeval = obj_new(struct timeval,NULL);
    s->p_timeval->tv_sec = 0; // no seconds
    s->p_timeval->tv_usec =  1000*msecs;  // microseconds        
}

bool select_do_later_again(Select *s, int msecs, SelectTimerProc callback, void *data, bool once_off) {
    if (s->milli_timer != NULL) // we're already waiting...
        return false;
    SelectTimer *st = obj_new(SelectTimer,NULL);
    st->s = s;
    s->once_off = once_off;
    s->milli_timer = st;
    st->callback = callback;
    st->data = data;
    select_set_timeout(s,msecs);
    return true;
}

bool select_do_later(Select *s, int msecs, SelectTimerProc callback, void *data) {
    return select_do_later_again(s,msecs,callback,data,true);
}

static void handle_timeout(Select *s) {
    s->milli_timer->callback(s->milli_timer->data);
    obj_unref(s->milli_timer);
    obj_unref(s->p_timeval);
    s->milli_timer = NULL;
    s->p_timeval = NULL;
    s->once_off = false;
}

#define LINE_SIZE 256

int select_select(Select *s) {
    fd_set *fds_r = &s->fdset_read;
    fd_set *fds_w = &s->fdset_write;
    s->error = false;
    
top:
    
    FD_ZERO(fds_r);
    FD_ZERO(fds_w);
    
    FOR_LIST(item,s->fds) {
        SelectFile *sf = SFILE(item);
        if (sf->writing) {
            printf("writing %d\n",sf->fd);
            FD_SET(sf->fd,fds_w);
        } else {
            FD_SET(sf->fd,fds_r);
        }
    }
    
    int res = select(s->nfds,fds_r,fds_w,NULL,s->p_timeval);
    if (res < 0) {
        s->error = true;
        return -1;
    }
    
    if (res == 0) { // timeout
        if (s->milli_timer) {
            handle_timeout(s);
            goto top;
        } else { // no callback            
            return 0;
        }
    }
    
    FOR_LIST(item,s->fds) {
        SelectFile *sf = SFILE(item);
        if (sf->handler && select_can_read(s,sf->fd)) {
            char buff[LINE_SIZE];
            int len = read(sf->fd,buff,LINE_SIZE);
            if (len == 0) { // ctrl+D
                if (sf->exit_on_close) {
                    return -1;
                } else { // otherwise we'll just remove the file and close it
                    close(sf->fd);
                    if (sf->flags & SelectReOpen) { // unless we want to simply re-open it
                        sf->fd = select_open_file(sf->name,sf->flags);
                    } else {
                        select_remove_read(s,sf->fd);
                    }
                    break;
                }
            }
            buff[len-1] = '\0'; // strip \n
            int res = sf->handler(buff,sf->handler_data);
            if (sf->exit_on_close && res != 0)
                return -1;
            goto top;
        }
    }
    
    if (s->timers && list_size(s->timers) > 0 && select_can_read_chan(s,s->tchan)) {
        byte id;
        SelectTimer *finis = NULL;
        chan_read(s->tchan,&id,1);
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

bool select_can_read_chan(Select *s, SelectChan *chan) {
    return select_can_read(s,chan->tread);
}


bool select_can_write_chan(Select *s, SelectChan *chan) {
    return select_can_write(s,chan->twrite);
}

