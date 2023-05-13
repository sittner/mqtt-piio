#ifndef _MQTT_H_
#define _MQTT_H_

#include <stdint.h>
#include <sys/select.h>
#include <confuse.h>

typedef struct {
  const char *host;
  int port;
  const char *client_id;
  const char *username;
  const char *password;
  const char *base_topic;

  const char *status_topic;
} MQTT_CONF_DATA_T;

extern MQTT_CONF_DATA_T mqtt_conf;

void mqtt_init(void);
int mqtt_configure(cfg_t *cfg);
void mqtt_unconfigure(void);

int mqtt_startup(void);
void mqtt_shutdown(void);

int mqtt_publish_switch(const char *topic, int val);
int mqtt_publish_int(const char *topic, int val);

#endif

