#ifndef _PIIO_CONF_H_
#define _PIIO_CONF_H_

#include "switch.h"
#include "rollershutter.h"

extern SWITCH_DATA_T *piio_conf_switch;
extern int piio_conf_switch_count;

extern ROLLSH_DATA_T *piio_conf_rollsh;
extern int piio_conf_rollsh_count;

int piio_conf_load(const char *file);
void piio_conf_cleanup(void);

#endif

