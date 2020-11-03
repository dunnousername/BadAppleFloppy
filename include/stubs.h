#ifndef STUBS_H
#define STUBS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

static bool _do_debug_messages = false;

void set_debug(bool speak) {
    printf("[ INFO] debug messages will be %s.\n", speak ? "shown" : "hidden");
    _do_debug_messages = speak;
}

void debug_msg(char *msg) {
    if (_do_debug_messages) {
        printf("[DEBUG] %s\n", msg);
    }
}

void info_msg(char *msg) {
    printf("[ INFO] %s\n", msg);
}

void panic() {
    info_msg("panicking!");
    exit(1);
}

void panic_msg(char *msg) {
    debug_msg(msg);
    panic();
}

#endif