#ifndef _SWITCH_H
#define _SWITCH_H

#include <confuse.h>
#include <gpiod.h>

#define SWITCH_CMD_NOP  0
#define SWITCH_CMD_ON   1
#define SWITCH_CMD_OFF  2

typedef struct {
  const char *name;
  int pin;

  const char *cmd_topic;
  const char *state_topic;

  const char *pin_name;
  struct gpiod_line *pin_line;

  int cmd;
  int do_restore;

  int output;
  int on_time;
  int on_timer;
} SWITCH_DATA_T;

int switch_init(cfg_t *cfg, SWITCH_DATA_T *sw);
void switch_cleanup(SWITCH_DATA_T *sw);

void switch_cmd(SWITCH_DATA_T *sw, const char *cmd);
void switch_restore(SWITCH_DATA_T *sw, const char *cmd);

void switch_period(SWITCH_DATA_T *sw);

#endif

