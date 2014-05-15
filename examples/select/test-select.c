#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "select.h"

#define LINE_SIZE 256

int main()
{
   
    int pid =  select_thread(callback,NULL);
    
    
    wait(pid);
    
    return 0;
    
        
}
