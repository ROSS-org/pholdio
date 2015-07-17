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
 * PHOLDIO Global Variable Declarations
 */
extern tw_stime lookahead;
extern static unsigned int stagger;
extern static unsigned int offset_lpid;
extern static tw_stime mult;
extern static tw_stime percent_remote;
extern static unsigned int ttl_lps;
extern static unsigned int nlp_per_pe;
extern static int g_pholdio_start_events;
extern static int optimistic_memory;
extern int io_store;

// rate for timestamp exponential distribution
extern static tw_stime mean;
extern static char run_id[1024];

#endif
