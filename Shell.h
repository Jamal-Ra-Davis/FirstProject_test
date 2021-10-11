#ifndef SHELL_LIB_
#define SHELL_LIB_

#include <stdio.h>

typedef enum {
    TRIANGLE, SQUARE, CROSS, CIRCLE,
    LBUMP, RBUMP, LSTICK, RSTICK,
    SHARE, OPTIONS, DUP, DLEFT,
    DDOWN, DRIGHT, DX, DY, NUM_KEYS
} ds4_keys_t;

typedef enum {
    POV_SCRATCH_LOOP,
    POV_TEST,
    DS4_TEST,
    SPACE_GAME,
    CLOCK_DISPLAY,
    NUM_POV_STATES
} pov_state_t;

#define LOG_POV_SHELL(shell, ...) rintf("SHELL::");printf(__VA_ARGS__)
#define SERIAL_PRINTF(ser, ...) printf("SERIAL::");printf(__VA_ARGS__)

#endif
