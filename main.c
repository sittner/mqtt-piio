#include <stdint.h>

#include "piio_conf.h"
#include "timer.h"
#include "mqtt.h"

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
  int err;
  struct sigaction act;

  // install signal handler
  exit_flag = false;
  memset(&act, 0, sizeof(act));
  act.sa_handler = &sighandler;
  sigaction(SIGINT, &act, NULL);
  sigaction(SIGTERM, &act, NULL);
  sigaction(SIGHUP, &act, NULL);

  // TODO: parameter
  if (piio_conf_load("piio.conf") < 0) {
    goto fail0;
  }

  if (timer_startup() < 0) {
    goto fail3;
  }

  if (mqtt_startup() < 0) {
    goto fail4;
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
      goto fail6;
    }

    // check task timers
    if (timer_handler(&read_fd_set) < 0) {
      goto fail6;
    }

  }
    
  ret = 0;

fail6:
  mqtt_shutdown();
fail4:
  timer_shutdown();
fail3:
  piio_conf_cleanup();
fail0:
  return ret;
}

