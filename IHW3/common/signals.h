#ifndef SIGNALS_H
#define SIGNALS_H

#include <signal.h>
#include <stdint.h>

extern volatile sig_atomic_t g_stop;

void setup_sigint(const char *role);

#endif

