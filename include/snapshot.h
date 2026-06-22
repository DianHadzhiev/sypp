#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include <signal.h>
#include <stdio.h>
#include "ijvm_struct.h"

//
extern volatile sig_atomic_t stop_requested;

void handle_sigint(int sig);
void save_snapshot(ijvm* m, const char *path);
ijvm* load_snapshot(const char *path, FILE *input, FILE *output);

#endif

