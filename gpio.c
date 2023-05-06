#include "gpio.h"

#include <gpio.h>
#include <string.h>
#include <syslog.h>

#define MAX_CHIP_COUNT 8

struct gpiod_chip *chips[MAX_CHIP_COUNT];

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

struct gpiod_line *gpio_request_output(int gpio, const char *name) {
  int chip_no, line_no;
  struct gpiod_chip *chip;
  struct gpiod_line *line;

  chip_no = gpio / 32;
  line_no = gpio % 32;

  chip = get_chip_by_number(chip_no);
  if (chip == NULL) {
    return NULL;
  }

  line = gpiod_chip_get_line(chip, line_no);
  if (line == NULL) {
    syslog(LOG_ERR, "Failed to get line %d:%d for gpio %d.", chip_no, line_no, gpio);
    return NULL;
  }

  if (gpiod_line_request_output(line, name, 0) < 0) {
    syslog(LOG_ERR, "Failed to request output line %d:%d for gpio %d.", chip_no, line_no, gpio);
    return NULL;
  }

  return line;
}

