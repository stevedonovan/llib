#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <llib/str.h>
#include <llib/map.h>
#include "select.h"

void prompt()
{
    printf("> ");
    fflush(stdout);
}

static Select *s;
static Map *timers;
static List *list;

bool timer_fun(const char *name) {
    fprintf(stderr,"timer '%s'\n",name);
    return true;
}

int safe_atoi(const char *s) {
    if (s == NULL)
        return 0;
    else
        return atoi(s);
}

void dump_list(List *list,const char *fmt) {
    FOR_LIST(item,list) {
        printf(fmt,(int)item->data);
    }
    printf("\n");
}

static Map *map;

void count() {
    printf("kount = %d\n",obj_kount());
}

int commands(const char *line) {
    char **words = str_split(line," ");
    int res = 0;
    if (array_len(words) == 0) {
        goto end;
    }    
    char *cmd = words[0];
    if (str_eq(cmd,"quit")) {
        res = 1;
    } else
    if (str_eq(cmd,"put")) {
        char *key = ref(words[1]);
        char *val = words[2];
        if (val == NULL) {
            map_delete(map,key);
        } else {
            map_put(map,key,ref(val));
        }
        FOR_MAP(iter,map) {
            printf("[%s]%s (%d) ",iter->key,iter->value,obj_refcount(iter->value));
        }
        printf("\n");
    } else
    if (str_eq(cmd,"get")) {
        printf("%s\n",map_get(map,words[1]));
    } else
    if (str_eq(cmd,"add")) {
        list_add (list, (void*) safe_atoi(words[1]));
    } else
    if (str_eq(cmd,"list")) {
        dump_list(list,"%d ");
    } else
    if (str_eq(cmd,"break")) {
        printf("break!\n");
    } else
    if (str_eq(cmd,"del")) {
        ListIter iter = list_find(list,(void*)safe_atoi(words[1]));
        if (! iter) {
            printf("can't find this element\n");
        } else {
            list_remove(list,iter);
            dump_list(list,"%d ");            
        }
    } else
    if (str_eq(cmd,"timer")) {
        char *name = ref(words[1]);
        int secs = safe_atoi(words[2]) ;
        if (secs == 0) {
            printf("timer <name>   <number of seconds>\n");
        } else {
            SelectTimer *st = map_get(timers,name);
            printf("name '%s' secs %d\n",name,secs);
            if (st != NULL) {
                select_timer_kill(st);
            }
            st = select_add_timer(s,secs,(SelectTimerProc)timer_fun,name);
            map_put(timers,name,st);
        }
    } else
    if (str_eq(cmd,"all")) {
        char *name;
        SelectTimer *st;
        FOR_MAP_KEYVALUE(name,st,timers) {
            printf("timer %s \n",name);
        }
    } else
    if (str_eq(cmd,"kill")) {
        char *name = words[1];
        SelectTimer *st = map_get(timers,name);
        if (!st) {
            printf("no timer with name '%s'\n",name);
        } else {
            select_timer_kill(st);
            map_remove(timers,name);
        }        
    } else {
        printf("I don't know '%s' with %d args\n",cmd,array_len(words)-1);
    }
end:
    unref(words);
    prompt();
    return res;
}

#define LINE_SIZE 256

int main()
{
    s = select_new();
    timers = map_new_str_ptr();
    list = list_new_ptr();
    map = map_new_str_str();
    
    int fd = open("/tmp/mypipe",O_RDONLY | O_NONBLOCK  );    
    select_add_read(s,fd);
    printf("added %d\n",fd);
    
    select_add_reader(s, commands);
    
    prompt();
    
    while(1) {
        int res = select_select(s);
        if (res == -1) {
          //  if (! s->error)
                perror("select");
            break;
        } else 
        if (fd > -1 && select_can_read(s,fd)) {
            char buff[LINE_SIZE];
            int len = read(fd,buff,LINE_SIZE);     
            if (len == 0) {
                // end of file?
                printf("finished with that\n");
                bool res = select_remove_read(s,fd);
                printf("and returned %d\n",res);
                close(fd);
                fd = -1;
                //printf("ids: "); dump_list(s->fds,"%d ");
            } else {
                buff[len-1] = '\0'; // strip \n
                fprintf(stderr,"Got '%s'\n",buff);
            }
        }
    }
    
    dispose(s, list);
    if (fd != -1) close(fd);
    dispose(map);
    count();
}
