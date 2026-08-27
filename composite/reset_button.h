#ifndef RESET_BUTTON_H
#define RESET_BUTTON_H

#include <stdbool.h>

void reset_button_init(void);
bool reset_button_poll(void);

#endif // RESET_BUTTON_H
