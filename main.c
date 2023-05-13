#include <stdint.h>

#include "gpio.h"
#include "piio_conf.h"
#include "timer.h"
#include "mqtt.h"
#include "switch.h"
#include "rollershutter.h"

#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <syslog.h>
#include <signal.h>
#include <sys/timerfd.h>
#include <sys/eventfd.h>

#define DEFAULT_CFG_FILE "/etc/piio.conf"

static bool exit_flag;

static void sighandler(int sig)
{
  switch (sig)
  {
    case SIGINT:
    case SIGTERM:
      exit_flag = true;
      break;
    case SIGHUP:
      break;
    default:
      syslog(LOG_DEBUG, "Unhandled signal %d", sig);
  }
}

int main(int argc, char **argv)
{
  int ret = 1;
  const char *cfg_file;
  int err;
  struct sigaction act;

  cfg_file = DEFAULT_CFG_FILE;
  if (argc >= 2) {
    cfg_file = argv[1];
  }

  // install signal handler
  exit_flag = false;
  memset(&act, 0, sizeof(act));
  act.sa_handler = &sighandler;
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGHUP, &act, NULL);

  if (gpio_init() < 0) {
    goto fail_gpio;
  }

  if (piio_conf_load(cfg_file) < 0) {
    goto fail_conf;
  }

  if (switch_startup() < 0) {
    goto fail_switch;
  }

  if (rollsh_startup() < 0) {
    goto fail_rollsh;
  }

  if (timer_startup() < 0) {
    goto fail_timer;
  }

  if (mqtt_startup() < 0) {
    goto fail_mqtt;
  }

  while(!exit_flag) {
    fd_set read_fd_set;
    FD_ZERO(&read_fd_set);
    timer_update_fds(&read_fd_set);

    err = select(FD_SETSIZE, &read_fd_set, NULL, NULL, NULL);
    if (err < 0) {
      if (errno == EINTR) {
        continue;
      }
      syslog(LOG_ERR, "Failed on socket select (error %d)", errno);
      goto fail;
    }

    // check task timers
    if (timer_handler(&read_fd_set) < 0) {
      goto fail;
    }

  }

  ret = 0;

fail:
  mqtt_shutdown();
fail_mqtt:
  timer_shutdown();
fail_timer:
  rollsh_shutdown();
fail_rollsh:
  switch_shutdown();
fail_switch:
  piio_conf_cleanup();
fail_conf:
  gpio_cleanup();
fail_gpio:
  return ret;
}

