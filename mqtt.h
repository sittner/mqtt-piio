#ifndef _MQTT_H_
#define _MQTT_H_

#include <stdint.h>
#include <sys/select.h>

typedef struct {
  const char *topic;
  const char *host;
  int port;
  const char *client_id;
  const char *username;
  const char *password;
} MQTT_CONF_DATA_T;

int mqtt_startup(const MQTT_CONF_DATA_T *conf);
void mqtt_shutdown(void);

#endif

