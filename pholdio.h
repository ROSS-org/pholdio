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
extern unsigned int stagger;
extern unsigned int offset_lpid;
extern tw_stime mult;
extern tw_stime percent_remote;
extern unsigned int ttl_lps;
extern unsigned int nlp_per_pe;
extern int g_pholdio_start_events;
extern int optimistic_memory;
extern int io_store;

// rate for timestamp exponential distribution
extern tw_stime mean;
extern char run_id[1024];

#endif
