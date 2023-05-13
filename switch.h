#ifndef _SWITCH_H
#define _SWITCH_H

#include <confuse.h>
#include <gpiod.h>
#include <mosquitto.h>

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

void switch_init(void);
int switch_configure(cfg_t *cfg);
void switch_unconfigure(void);

int switch_startup(void);
void switch_shutdown(void);

void switch_mqtt_subscribe(struct mosquitto *mosq);
void switch_mqtt_msg(const char *topic, const char *msg);

void switch_period(void);

#endif

