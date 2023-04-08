#ifndef _TIMER_H_
#define _TIMER_H_

#include <sys/select.h>

#define TIMER_PERIOD_MS 100

int timer_startup(void);
void timer_shutdown(void);
void timer_update_fds(fd_set *fd_set);
int timer_handler(fd_set *fd_set);

#endif

