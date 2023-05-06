#include "mqtt.h"
#include "piio_conf.h"
#include "switch.h"
#include "rollershutter.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <fcntl.h>
#include <mosquitto.h>

#define KEEPALIVE_PERIOD 300

#define CONST_STR_PAYLOAD(s) (sizeof(s) - 1), s

MQTT_CONF_DATA_T mqtt_conf;

static struct mosquitto *mosq = NULL;

static void connect_callback(struct mosquitto *mosq, void *obj, int result) {
  SWITCH_DATA_T *sw;
  ROLLSH_DATA_T *rollsh;
  int i;

  mosquitto_publish(mosq, NULL, mqtt_conf.status_topic, CONST_STR_PAYLOAD("ON"), 1, true);

  // subscribe switches
  for (i = 0, sw = piio_conf_switch; i < piio_conf_switch_count; i++, sw++) {
    mosquitto_subscribe(mosq, NULL, sw->cmd_topic, 0);
  }

  // subscribe rollershutters
  for (i = 0, rollsh = piio_conf_rollsh; i < piio_conf_rollsh_count; i++, rollsh++) {
    mosquitto_subscribe(mosq, NULL, rollsh->cmd_topic, 0);
  }
}

static void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg) {
  char buf[32];
  SWITCH_DATA_T *sw;
  ROLLSH_DATA_T *rollsh;
  int i;

  // check for maximum payload length
  if (msg->payloadlen >= (sizeof(buf) - 1)) {
    return;
  }

  // get payload as string
  memcpy(buf, msg->payload, msg->payloadlen);
  buf[msg->payloadlen] = 0;

  // process switches
  for (i = 0, sw = piio_conf_switch; i < piio_conf_switch_count; i++, sw++) {
    if (strcmp(sw->cmd_topic, msg->topic) == 0) {
      if (strcmp("ON", buf) == 0) {
        sw->cmd = 1;
      }
      if (strcmp("OFF", buf) == 0) {
        sw->cmd = 0;
      }
    }
  }

  // process rollershutters
  for (i = 0, rollsh = piio_conf_rollsh; i < piio_conf_rollsh_count; i++, rollsh++) {
    if (strcmp(rollsh->cmd_topic, msg->topic) == 0) {
      if (strcmp("OPEN", buf) == 0) {
        rollsh->cmd = -1;
      }
      if (strcmp("CLOSE", buf) == 0) {
        rollsh->cmd = 1;
      }
      if (strcmp("OFF", buf) == 0) {
        rollsh->cmd = 0;
      }
    }
  }
}

static char *strdup_null(const char *s) {
  if (s == NULL) {
    return NULL;
  }
  return strdup(s);
}

void mqtt_configure(cfg_t *cfg) {
  mqtt_conf.base_topic = strdup(cfg_getstr(cfg, "topic"));
  mqtt_conf.host = strdup(cfg_getstr(cfg, "host"));
  mqtt_conf.port = cfg_getint(cfg, "port");
  mqtt_conf.client_id = strdup_null(cfg_getstr(cfg, "client_id"));
  mqtt_conf.username = strdup_null(cfg_getstr(cfg, "user"));
  mqtt_conf.password = strdup_null(cfg_getstr(cfg, "passwd"));

  asprintf((char **) &mqtt_conf.status_topic, "%s/status", mqtt_conf.base_topic);
}

void mqtt_unconfigure(void) {
  free((void *) mqtt_conf.base_topic);
  free((void *) mqtt_conf.host);
  free((void *) mqtt_conf.client_id);
  free((void *) mqtt_conf.username);
  free((void *) mqtt_conf.password);
  free((void *) mqtt_conf.status_topic);
}

int mqtt_startup(void) {
  if (mosquitto_lib_init()) {
    syslog(LOG_ERR, "Failed to init mosquitto lib");
    goto fail0;
  }

  mosq = mosquitto_new(mqtt_conf.client_id, true, NULL);
  if (mosq == NULL) {
    syslog(LOG_ERR, "Failed to create mosquitto instance");
    goto fail1;
  }

  if (mqtt_conf.username != NULL) {
    if (mosquitto_username_pw_set(mosq, mqtt_conf.username, mqtt_conf.password)) {
      syslog(LOG_ERR, "Failed to set mosquitto credentials");
      goto fail2;
    }
  }

  mosquitto_connect_callback_set(mosq, connect_callback);
  mosquitto_message_callback_set(mosq, message_callback);

  if (mosquitto_will_set(mosq, mqtt_conf.status_topic, CONST_STR_PAYLOAD("OFF"), 1, true)) {
    syslog(LOG_ERR, "Failed to set mosquitto will");
    goto fail2;
  }

  if (mosquitto_connect_async(mosq, mqtt_conf.host, mqtt_conf.port, KEEPALIVE_PERIOD)) {
    syslog(LOG_INFO, "initial mqtt connection failed");
  }

  if (mosquitto_loop_start(mosq)) {
    syslog(LOG_ERR, "Failed to start mosquitto thread");
    goto fail3;
  }

  return 0;

fail3:
  mosquitto_disconnect(mosq);
fail2:
  mosquitto_destroy(mosq);
fail1:
  mosquitto_lib_cleanup();
fail0:
  return -1;
}

void mqtt_shutdown(void) {
  mosquitto_publish(mosq, NULL, mqtt_conf.status_topic, CONST_STR_PAYLOAD("OFF"), 1, true);
  mosquitto_disconnect(mosq);
  mosquitto_loop_stop(mosq, false);
  mosquitto_destroy(mosq);
  mosquitto_lib_cleanup();
}

int mqtt_publish_switch(const char *topic, int val) {
  if (val) {
    return mosquitto_publish(mosq, NULL, topic, CONST_STR_PAYLOAD("ON"), 1, true);
  } else {
    return mosquitto_publish(mosq, NULL, topic, CONST_STR_PAYLOAD("OFF"), 1, true);
  }
}

int mqtt_publish_int(const char *topic, int val) {
  char buf[32];
  int len;

  len = snprintf(buf, sizeof(buf), "%d", val);
  if (len > sizeof(buf)) {
    return MOSQ_ERR_PAYLOAD_SIZE;
  }

  return mosquitto_publish(mosq, NULL, topic, len, buf, 1, true);
}

