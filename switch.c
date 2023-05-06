#include "switch.h"
#include "mqtt.h"

#include <stdlib.h>
#include <string.h>

int switch_init(cfg_t *cfg, SWITCH_DATA_T *sw) {
  sw->name = strdup(cfg_title(cfg));
  sw->gpio = cfg_getint(cfg, "gpio");
  sw->on_time = cfg_getint(cfg, "on_time");

  asprintf((char **) &sw->cmd_topic, "%s/%s/cmd", mqtt_conf.base_topic, sw->name);
  asprintf((char **) &sw->state_topic, "%s/%s/state", mqtt_conf.base_topic, sw->name);

  return 0;
}

void switch_cleanup(SWITCH_DATA_T *sw) {
  free((void *) sw->name);
  free((void *) sw->cmd_topic);
  free((void *) sw->state_topic);
}

void switch_period(SWITCH_DATA_T *sw) {

}

