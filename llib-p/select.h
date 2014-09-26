#ifndef LLIB_SELECT_H
#define LLIB_SELECT_H

typedef bool (*SelectTimerProc)(void *data);
typedef int (*SelectReadProc)(const char *line,...);

typedef struct Select_ Select;
typedef struct SelectTimer_ SelectTimer;
typedef struct SelectFile_ SelectFile;
typedef struct SelectChan_ SelectChan;

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
bool select_can_write(Select *s, int fd);
bool select_can_read_chan(Select *s, SelectChan *chan);

SelectFile *select_add_write(Select *s, int fd);
SelectFile *select_add_write_chan(Select *s, SelectChan *chan);
bool select_can_write_chan(Select *s, SelectChan *chan);

int *select_read_fds(Select *s);

void select_add_reader(Select *s, int fd, bool close, SelectReadProc reader, void *data);

void select_timer_kill(SelectTimer *st);
void select_set_timeout(Select *s, int msecs);
SelectTimer *select_add_timer(Select *s, int secs, SelectTimerProc callback, void *data);
bool select_do_later(Select *s, int msec, SelectTimerProc callback, void *data);
bool select_do_later_again(Select *s, int msecs, SelectTimerProc callback, void *data, bool once_off);
int select_select(Select *s);

SelectChan *chan_new();
SelectFile *select_add_read_chan(Select *s, SelectChan *chan);
int chan_write(SelectChan *chan, void *buff, int sz);
int chan_read(SelectChan *chan, void *buff, int sz);
int chan_close_read(SelectChan *chan);
int chan_close_write(SelectChan *chan) ;


#endif

