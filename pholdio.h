#ifndef INC_pholdio_h
#define INC_pholdio_h

#include <ross.h>
#include "pholdio-config.h"

/*
 * PHOLDIO Types
 */
typedef struct pholdio_state pholdio_state;
typedef struct pholdio_message pholdio_message;

struct pholdio_state {
    long int dummy_state;
};

struct pholdio_message {
    long int dummy_data;
};

/*
 * PHOLDIO Globals
 */
tw_stime lookahead = 1.0;
static unsigned int stagger = 0;
static unsigned int offset_lpid = 0;
static tw_stime mult = 1.4;
static tw_stime percent_remote = 0.25;
static unsigned int ttl_lps = 0;
static unsigned int nlp_per_pe = 8;
static int g_pholdio_start_events = 1;
static int optimistic_memory = 100;
int io_store = 0;

// rate for timestamp exponential distribution
static tw_stime mean = 1.0;

static char run_id[1024] = "undefined";

#endif
