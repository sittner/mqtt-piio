#include "rollershutter.h"
#include "mqtt.h"

#include <stdlib.h>
#include <string.h>

int rollsh_init(cfg_t *cfg, ROLLSH_DATA_T *rollsh) {
  rollsh->name = strdup(cfg_title(cfg));
  rollsh->gpio_up = cfg_getint(cfg, "gpio_up");
  rollsh->gpio_down = cfg_getint(cfg, "gpio_down");
  rollsh->time_full = cfg_getint(cfg, "time_full");
  rollsh->time_extra = cfg_getint(cfg, "time_extra");
  rollsh->time_pause = cfg_getint(cfg, "time_pause");

  asprintf((char **) &rollsh->cmd_topic, "%s/%s/cmd", mqtt_conf.base_topic, rollsh->name);
  asprintf((char **) &rollsh->state_topic, "%s/%s/state", mqtt_conf.base_topic, rollsh->name);

  return 0;
}

void rollsh_cleanup(ROLLSH_DATA_T *rollsh) {
  free((void *) rollsh->name);
  free((void *) rollsh->cmd_topic);
  free((void *) rollsh->state_topic);
}

void rollsh_period(ROLLSH_DATA_T *rollsh) {

}

