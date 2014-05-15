#ifndef LLIB_SELECT_H
#define LLIB_SELECT_H

typedef bool (*SelectTimerProc)(void *data);
typedef int (*SelectReadProc)(const char *line,...);

typedef struct Select_ Select;
typedef struct SelectTimer_ SelectTimer;
typedef struct SelectFile_ SelectFile;
typedef struct Pipe_ Pipe;

enum {
    SelectRead = 1,
    SelectWrite = 2,
    SelectReadWrite = 4,
    SelectNonBlock = 8,
    SelectReOpen = 16
};

Select *select_new();
int select_thread(SelectTimerProc callback, void *data);
void select_sleep(int msec);
SelectFile *select_add_read(Select *s, int fd);
int select_open(Select *s, const char *str, int flags);
bool select_remove_read(Select *s, int fd);
bool select_can_read(Select *s, int fd);
bool select_can_read_pipe(Select *s, Pipe *pipe);
int *select_read_fds(Select *s);

void select_add_reader(Select *s, int fd, bool close, SelectReadProc reader, void *data);

void select_timer_kill(SelectTimer *st);
SelectTimer *select_add_timer(Select *s, int secs, SelectTimerProc callback, void *data);
bool select_do_later(Select *s, int msec, SelectTimerProc callback, void *data);
int select_select(Select *s);

Pipe *pipe_new();
int pipe_write(Pipe *pipe, void *buff, int sz);
int pipe_read(Pipe *pipe, void *buff, int sz);

#endif

