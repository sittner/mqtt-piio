#include "rollershutter.h"
#include "mqtt.h"
#include "gpio.h"
#include "timer.h"

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

int rollsh_init(cfg_t *cfg, ROLLSH_DATA_T *rollsh) {
  rollsh->name = strdup(cfg_title(cfg));
  rollsh->pin_up = cfg_getint(cfg, "pin_up");
  rollsh->pin_down = cfg_getint(cfg, "pin_down");
  rollsh->time_full = cfg_getint(cfg, "time_full");
  rollsh->time_extra = cfg_getint(cfg, "time_extra");
  rollsh->time_pause = cfg_getint(cfg, "time_pause");

  asprintf((char **) &rollsh->cmd_topic, "%s/%s/cmd", mqtt_conf.base_topic, rollsh->name);
  asprintf((char **) &rollsh->state_topic, "%s/%s/state", mqtt_conf.base_topic, rollsh->name);

  asprintf((char **) &rollsh->pin_up_name, "piio.%s.up", rollsh->name);
  rollsh->pin_up_line = gpio_request_output(rollsh->pin_up, rollsh->pin_up_name);
  if (rollsh->pin_up_line == NULL) {
    return -1;
  }

  asprintf((char **) &rollsh->pin_down_name, "piio.%s.down", rollsh->name);
  rollsh->pin_down_line = gpio_request_output(rollsh->pin_down, rollsh->pin_down_name);
  if (rollsh->pin_down_line == NULL) {
    return -1;
  }

  rollsh->do_restore = 1;
  rollsh->last_pos = -1;

  return 0;
}

void rollsh_cleanup(ROLLSH_DATA_T *rollsh) {
  free((void *) rollsh->name);
  free((void *) rollsh->cmd_topic);
  free((void *) rollsh->state_topic);
  free((void *) rollsh->pin_up_name);
  free((void *) rollsh->pin_down_name);

  if (rollsh->pin_up_line != NULL) {
    gpiod_line_release(rollsh->pin_up_line);
  }

  if (rollsh->pin_down_line != NULL) {
    gpiod_line_release(rollsh->pin_down_line);
  }
}

void rollsh_cmd(ROLLSH_DATA_T *rollsh, const char *cmd) {
  rollsh->do_restore = 0;

  if (strcmp("OPEN", cmd) == 0) {
    rollsh->cmd = ROLLSH_CMD_OPEN;
  }
  if (strcmp("CLOSE", cmd) == 0) {
    rollsh->cmd = ROLLSH_CMD_CLOSE;
  }
  if (strcmp("OFF", cmd) == 0) {
    rollsh->cmd = ROLLSH_CMD_OFF;
  }
}

static int limit_pct(int val) {
  if (val < 0) {
    return 0;
  }

  if (val > 100) {
    return 100;
  }

  return val;
}

void rollsh_restore(ROLLSH_DATA_T *rollsh, const char *cmd) {
  if (!rollsh->do_restore) {
    return;
  }

  rollsh->do_restore = 0;
  rollsh->position = limit_pct(atoi(cmd)) * rollsh->time_full / 100;
}

static void handle_cmd(ROLLSH_DATA_T *rollsh, int cmd, int other, int *timer) {
  if (rollsh->output == other) {
    *timer = rollsh->time_pause;
  }

  if (*timer > 0) {
    *timer -= TIMER_PERIOD_MS;
    return;
  }

  if (rollsh->cmd == cmd) {
    rollsh->cmd = ROLLSH_CMD_NOP;
    rollsh->output = cmd;
  }
}

void rollsh_period(ROLLSH_DATA_T *rollsh) {
  int pos;

  // handle commands
  handle_cmd(rollsh, ROLLSH_CMD_CLOSE, ROLLSH_CMD_OPEN, &rollsh->down_timer);
  handle_cmd(rollsh, ROLLSH_CMD_OPEN, ROLLSH_CMD_CLOSE, &rollsh->up_timer);
  if (rollsh->cmd != ROLLSH_CMD_NOP) {
    rollsh->output = ROLLSH_CMD_NOP;
  }
  if (rollsh->cmd == ROLLSH_CMD_OFF) {
    rollsh->cmd = ROLLSH_CMD_NOP;
  }

  // update position
  if (rollsh->output == ROLLSH_CMD_CLOSE) {
    rollsh->position += TIMER_PERIOD_MS;
    if (rollsh->position >= (rollsh->time_full + rollsh->time_extra)) {
      rollsh->output = ROLLSH_CMD_NOP;
      rollsh->position = rollsh->time_full;
    }
  }
  if (rollsh->output == ROLLSH_CMD_OPEN) {
    rollsh->position -= TIMER_PERIOD_MS;
    if (rollsh->position <= (0 - rollsh->time_extra)) {
      rollsh->output = ROLLSH_CMD_NOP;
      rollsh->position = 0;
    }
  }

  // report state
  pos = 0;
  if (rollsh->time_full > 0) {
    pos = limit_pct(rollsh->position * 100 / rollsh->time_full);
  }
  if (!rollsh->do_restore && pos != rollsh->last_pos) {
    rollsh->last_pos = pos;
    mqtt_publish_int(rollsh->state_topic, pos);
  }

  // set putputs
  gpiod_line_set_value(rollsh->pin_up_line, (rollsh->output == ROLLSH_CMD_OPEN));
  gpiod_line_set_value(rollsh->pin_down_line, (rollsh->output == ROLLSH_CMD_CLOSE));
}

