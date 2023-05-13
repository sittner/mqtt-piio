#include "switch.h"
#include "mqtt.h"
#include "gpio.h"
#include "timer.h"
#include "piio_conf.h"

#include <stdlib.h>
#include <string.h>
#include <syslog.h>

static int switches_count;
static SWITCH_DATA_T *switches;

static int configure_instance(cfg_t *cfg, void *ctx, void *child) {
  SWITCH_DATA_T *sw = (SWITCH_DATA_T *) child;

  sw->name = piio_conf_strdup(cfg_title(cfg));
  sw->pin = cfg_getint(cfg, "pin");
  sw->on_time = cfg_getint(cfg, "on_time");

  asprintf((char **) &sw->cmd_topic, "%s/%s/cmd", mqtt_conf.base_topic, sw->name);
  asprintf((char **) &sw->state_topic, "%s/%s/state", mqtt_conf.base_topic, sw->name);

  asprintf((char **) &sw->pin_name, "piio.%s", sw->name);

  return 0;
}

void switch_init(void) {
  switches_count = 0;
  switches = NULL;
}

int switch_configure(cfg_t *cfg) {
  return piio_conf_config_childs(cfg, "switch", &switches_count, (void *) &switches, sizeof(SWITCH_DATA_T), NULL, configure_instance);
}

void switch_unconfigure(void) {
  int i;
  SWITCH_DATA_T *sw;

  for (i = 0, sw = switches; i < switches_count; i++, sw++) {
    free((void *) sw->name);
    free((void *) sw->cmd_topic);
    free((void *) sw->state_topic);
    free((void *) sw->pin_name);
  }
  free((void *) switches);
}

int switch_startup(void) {
  int i;
  SWITCH_DATA_T *sw;

  for (i = 0, sw = switches; i < switches_count; i++, sw++) {
    sw->pin_line = gpio_request_output(sw->pin, sw->pin_name);
    if (sw->pin_line == NULL) {
      return -1;
    }

    sw->do_restore = 1;
  }

  return 0;
}

void switch_shutdown(void) {
  int i;
  SWITCH_DATA_T *sw;

  for (i = 0, sw = switches; i < switches_count; i++, sw++) {
    if (sw->pin_line != NULL) {
      gpiod_line_release(sw->pin_line);
    }
  }
}

void switch_mqtt_subscribe(struct mosquitto *mosq) {
  int i;
  SWITCH_DATA_T *sw;

  for (i = 0, sw = switches; i < switches_count; i++, sw++) {
    mosquitto_subscribe(mosq, NULL, sw->cmd_topic, 0);
    mosquitto_subscribe(mosq, NULL, sw->state_topic, 0);
  }
}

static void instance_cmd(SWITCH_DATA_T *sw, const char *cmd) {
  sw->do_restore = 0;

  if (strcmp("ON", cmd) == 0) {
    sw->cmd = SWITCH_CMD_ON;
  }
  if (strcmp("OFF", cmd) == 0) {
    sw->cmd = SWITCH_CMD_OFF;
  }
}

static void instance_restore(SWITCH_DATA_T *sw, const char *cmd) {
  if (!sw->do_restore) {
    return;
  }

  instance_cmd(sw, cmd);
}

void switch_mqtt_msg(const char *topic, const char *msg) {
  int i;
  SWITCH_DATA_T *sw;

  for (i = 0, sw = switches; i < switches_count; i++, sw++) {
    if (strcmp(sw->cmd_topic, topic) == 0) {
      instance_cmd(sw, msg);
    }
    // restore last state
    if (strcmp(sw->state_topic, topic) == 0) {
      instance_restore(sw, msg);
    }
  }
}

static void instance_period(SWITCH_DATA_T *sw) {
  int last_output;

  last_output = sw->output;
  if (sw->cmd == SWITCH_CMD_ON) {
    sw->output = 1;
  }
  if (sw->cmd == SWITCH_CMD_OFF) {
    sw->output = 0;
  }
  sw->cmd = SWITCH_CMD_NOP;

  if (sw->output && sw->on_timer > 0) {
    sw->on_timer -= TIMER_PERIOD_MS;
    if (sw->on_timer <= 0) {
      sw->output = 0;
    }
  }
  if (!sw->output) {
    sw->on_timer = sw->on_time;
  }

  if (sw->output != last_output) {
    mqtt_publish_switch(sw->state_topic, sw->output);
  }

  gpiod_line_set_value(sw->pin_line, sw->output);
}

void switch_period(void) {
  int i;
  SWITCH_DATA_T *sw;

  for (i = 0, sw = switches; i < switches_count; i++, sw++) {
    instance_period(sw);
  }
}
