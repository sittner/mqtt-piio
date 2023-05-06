#include "gpio.h"

#include <gpio.h>
#include <string.h>
#include <syslog.h>

#define MAX_CHIP_COUNT 8

struct gpiod_chip *chips[MAX_CHIP_COUNT];

#define PIN_COUNT 40

static const int pinmap[PIN_COUNT] = {
 -1, -1,
  2, -1,
  3, -1,
  4, 14,
 -1, 15,
 17, 18,
 27, -1,
 22, 23,
 -1, 24,
 10, -2,
  9, 25,
 11,  8,
 -1,  7,
 -1, -1,
  5, -1,
  6, 12,
 13, -1,
 19, 16,
 26, 20,
 -1, 21
};

int gpio_init(void) {
  memset(chips, 0, sizeof(chips));

  return 0;
}

void gpio_cleanup(void) {
  int i;
  struct gpiod_chip *chip;

  // release chips
  for (i = 0; i < MAX_CHIP_COUNT; i++) {
    chip = chips[i];
    if (chip != NULL) {
      gpiod_chip_close(chip);
    }
  }
}

static struct gpiod_chip *get_chip_by_number(int num) {
  struct gpiod_chip *chip;

  // check chip number
  if (num >= MAX_CHIP_COUNT) {
    syslog(LOG_ERR, "Invalid chip number %d.", num);
    return NULL;
  }

  // check if chip is allready opened
  chip = chips[num];
  if (chip != NULL) {
    return chip;
  }

  // open chip
  chip = gpiod_chip_open_by_number(num);
  if (chip == NULL) {
    syslog(LOG_ERR, "Failed to open gpio chip %d.", num);
    return NULL;
  }

  chips[num] = chip;
  return chip;
}

struct gpiod_line *gpio_request_output(int pin, const char *name) {
  int gpio, chip_no, line_no;
  struct gpiod_chip *chip;
  struct gpiod_line *line;

  gpio = -1;
  if (pin >= 1 && pin <= PIN_COUNT) {
    gpio = pinmap[pin - 1];
  }
  if (gpio < 0) {
    syslog(LOG_ERR, "Infalid gpio pin number %d.", pin);
    return NULL;
  }

  chip_no = gpio / 64;
  line_no = gpio % 64;

  chip = get_chip_by_number(chip_no);
  if (chip == NULL) {
    return NULL;
  }

  line = gpiod_chip_get_line(chip, line_no);
  if (line == NULL) {
    syslog(LOG_ERR, "Failed to get line %d:%d for pin %d.", chip_no, line_no, pin);
    return NULL;
  }

  if (gpiod_line_request_output(line, name, 0) < 0) {
    syslog(LOG_ERR, "Failed to request output line %d:%d for pin %d.", chip_no, line_no, pin);
    return NULL;
  }

  return line;
}

