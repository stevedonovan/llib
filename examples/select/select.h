#ifndef LLIB_SELECT_H
#define LLIB_SELECT_H

typedef bool (*SelectTimerProc)(void *data);
typedef int (*SelectReadProc)(const char *line);

typedef struct Select_ Select;
typedef struct SelectTimer_ SelectTimer;

enum {
    SelectRead = 1,
    SelectWrite = 2,
    SelectReadWrite = 4,
    SelectNonBlock = 8
};

Select *select_new();
void select_add_read(Select *s, int fd);
int select_open(Select *s, const char *str, int flags);
bool select_remove_read(Select *s, int fd);
bool select_can_read(Select *s, int fd);
//List *select_read_fds(Select *s);

void select_add_reader(Select *s, SelectReadProc reader);

void select_timer_kill(SelectTimer *st);
SelectTimer *select_add_timer(Select *s, int secs, SelectTimerProc callback, void *data);
bool select_do_later(Select *s, int msec, SelectTimerProc callback, void *data);
int select_select(Select *s);

#endif

