#include "pholdio.h"

// PHOLDIO Global Variable Definitions & default values

tw_stime lookahead = 1.0;
unsigned int stagger = 0;
unsigned int offset_lpid = 0;
tw_stime mult = 1.4;
tw_stime percent_remote = 0.25;
unsigned int ttl_lps = 0;
unsigned int nlp_per_pe = 8;
int g_pholdio_start_events = 1;
int optimistic_memory = 100;
int io_store = 0;

// rate for timestamp exponential distribution
tw_stime mean = 1.0;
char run_id[1024] = "undefined";

/**
 * Functions for RIO
 **/
void pholdio_serialize (pholdio_state *s, void *buffer, tw_lp *lp) {
    s->dummy_state = lp->gid + (100 * g_tw_mynode);
    printf("Storing Dummy %ld on %llu\n", s->dummy_state, lp->gid);
    memcpy(buffer, s, sizeof(pholdio_state));
    return;
}

void pholdio_deserialize (pholdio_state *s, void * buffer, tw_lp *lp) {
    memcpy(s, buffer, sizeof(pholdio_state));
    printf("Found Dummy %ld on %llu\n", s->dummy_state, lp->gid);
    return;
}

size_t pholdio_size (pholdio_state *s, tw_lp *lp) {
    return sizeof(pholdio_state);
}

tw_peid pholdio_map(tw_lpid gid) {
    return (tw_peid) gid / g_tw_nlp;
}

/**
 * Regular PHOLD functions
 **/
void pholdio_init(pholdio_state * s, tw_lp * lp) {
    int i;

    if ( stagger ) {
        for (i = 0; i < g_pholdio_start_events; i++) {
            double rand = tw_rand_exponential(lp->rng, mean);
            tw_event_send(tw_event_new(lp->gid, rand + lookahead + (tw_stime)(lp->gid % (unsigned int)g_tw_ts_end), lp));
        }
    } else {
        for (i = 0; i < g_pholdio_start_events; i++) {
            double rand = tw_rand_exponential(lp->rng, mean);
            tw_event_send(tw_event_new(lp->gid, rand + lookahead, lp));
        }
    }
}

void pholdio_event_handler(pholdio_state * s, tw_bf * bf, pholdio_message * m, tw_lp * lp) {
    tw_lpid dest;

    if (tw_rand_unif(lp->rng) <= percent_remote) {
        bf->c1 = 1;
        dest = tw_rand_integer(lp->rng, 0, ttl_lps - 1);
    } else {
        bf->c1 = 0;
        dest = lp->gid;
    }

    if(dest >= (g_tw_nlp * tw_nnodes())) {
        tw_error(TW_LOC, "bad dest");
    }
    double rand = tw_rand_exponential(lp->rng, mean);
    tw_event_send(tw_event_new(dest, rand + lookahead, lp));
}

void pholdio_event_handler_rc(pholdio_state * s, tw_bf * bf, pholdio_message * m, tw_lp * lp) {
    tw_rand_reverse_unif(lp->rng);
    tw_rand_reverse_unif(lp->rng);

    if(bf->c1 == 1) {
        tw_rand_reverse_unif(lp->rng);
    }
}

void pholdio_finish(pholdio_state * s, tw_lp * lp) {
}

tw_lptype mylps[] = {
    {(init_f) pholdio_init,
     (pre_run_f) NULL,
     (event_f) pholdio_event_handler,
     (revent_f) pholdio_event_handler_rc,
     (final_f) pholdio_finish,
     (map_f) pholdio_map,
    sizeof(pholdio_state)},
    {0},
};

io_lptype iolps[] = {
    {(serialize_f) pholdio_serialize,
     (deserialize_f) pholdio_deserialize,
     (model_size_f) pholdio_size},
     {0},
};

const tw_optdef app_opt[] = {
    TWOPT_GROUP("PHOLDIO Model"),
    TWOPT_STIME("remote", percent_remote, "desired remote event rate"),
    TWOPT_UINT("nlp", nlp_per_pe, "number of LPs per processor"),
    TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
    TWOPT_STIME("mult", mult, "multiplier for event memory allocation"),
    TWOPT_STIME("lookahead", lookahead, "lookahead for events"),
    TWOPT_UINT("start-events", g_pholdio_start_events, "number of initial messages per LP"),
    TWOPT_UINT("stagger", stagger, "Set to 1 to stagger event uniformly across 0 to end time."),
    TWOPT_UINT("memory", optimistic_memory, "additional memory buffers"),
    TWOPT_CHAR("run", run_id, "user supplied run name"),
    TWOPT_UINT("io-store", io_store, "io store checkpoint"),
    TWOPT_END()
};

int main(int argc, char **argv, char **env) {
    int i;

    // get rid of error if compiled w/ MEMORY queues
    g_tw_memory_nqueues=1;
    // set a min lookahead of 1.0
    lookahead = 1.0;
    tw_opt_add(app_opt);
    tw_init(&argc, &argv);

    if( lookahead > 1.0 ) {
        tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");
    }

    //reset mean based on lookahead
    mean = mean - lookahead;

    g_tw_memory_nqueues = 16; // give at least 16 memory queue event

    offset_lpid = g_tw_mynode * nlp_per_pe;
    ttl_lps = tw_nnodes() * g_tw_npe * nlp_per_pe;
    g_tw_events_per_pe = (mult * nlp_per_pe * g_pholdio_start_events) + optimistic_memory;
    //g_tw_rng_default = TW_FALSE;
    g_tw_lookahead = lookahead;

    tw_define_lps(nlp_per_pe, sizeof(pholdio_message), 0);

    g_tw_lp_types = mylps;
    g_io_lp_types = iolps;
    tw_lp_setup_types();

    if( g_tw_mynode == 0 ){
        printf("========================================\n");
        printf("PHOLDIO Model Configuration..............\n");
        printf("   Lookahead..............%lf\n", lookahead);
        printf("   Start-events...........%u\n", g_pholdio_start_events);
        printf("   stagger................%u\n", stagger);
        printf("   Mean...................%lf\n", mean);
        printf("   Mult...................%lf\n", mult);
        printf("   Memory.................%u\n", optimistic_memory);
        printf("   Remote.................%lf\n", percent_remote);
        printf("========================================\n\n");
    }

    tw_clock start;

    if (io_store != 2) {
        g_io_events_buffered_per_rank = 2*g_tw_nlp*g_pholdio_start_events;  // events past end time to store
        start = tw_clock_read();
        io_init(g_io_number_of_files, g_io_number_of_partitions);
        g_tw_pe[0]->stats.s_rio_load += (tw_clock_read() - start);
    }
    if (io_store == 0) {
        strcpy(g_io_checkpoint_name, "pholdio_checkpoint");
        g_io_load_at = INIT;
    }

    tw_run();

    if (io_store == 1) {
        start = tw_clock_read();
        io_register_model_version(MODEL_VERSION);
        io_store_checkpoint("pholdio_checkpoint");
        g_tw_pe[0]->stats.s_rio_load += (tw_clock_read() - start);
    }
    // io_final();

    tw_end();

    return 0;
}
