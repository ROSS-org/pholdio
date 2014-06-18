#include "phold.h"

void phold_datatype (MPI_Datatype *dt) {
    MPI_Datatype oldtypes[1];
    int blockcounts[1];
    MPI_Aint offsets[1];

    offsets[0] = 0;
    oldtypes[0] = MPI_LONG;
    blockcounts[0] = 1;

    MPI_Type_struct(1, blockcounts, offsets, oldtypes, dt);
    MPI_Type_commit(dt);
}

void phold_serialize (tw_lp *lp, void *store) {
    ((phold_state *)lp->cur_state)->dummy_state = lp->gid + (100 * g_tw_mynode);
    printf("Storing Dummy %ld on %lu\n", ((phold_state *)lp->cur_state)->dummy_state, lp->id);
    memcpy(store, lp->cur_state, sizeof(phold_state));
    return;
}

void phold_deserialize (void * store, tw_lp *lp) {
    memcpy(lp->cur_state, store, sizeof(phold_state));
    printf("Found Dummy %ld on %lu\n", ((phold_state *)lp->cur_state)->dummy_state, lp->gid);
    return;
}

tw_peid phold_map(tw_lpid gid) {
    return (tw_peid) gid / g_tw_nlp;
}

void phold_init(phold_state * s, tw_lp * lp) {
    int i;

    if ( stagger ) {
        for (i = 0; i < g_phold_start_events; i++) {
            double rand = tw_rand_exponential(lp->rng, mean);
            tw_event_send(tw_event_new(lp->gid, rand + lookahead + (tw_stime)(lp->gid % (unsigned int)g_tw_ts_end), lp));
        }
    } else {
        for (i = 0; i < g_phold_start_events; i++) {
            double rand = tw_rand_exponential(lp->rng, mean);
            tw_event_send(tw_event_new(lp->gid, rand + lookahead, lp));
        }
    }
}

void phold_event_handler(phold_state * s, tw_bf * bf, phold_message * m, tw_lp * lp) {
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

void phold_event_handler_rc(phold_state * s, tw_bf * bf, phold_message * m, tw_lp * lp) {
    tw_rand_reverse_unif(lp->rng);
    tw_rand_reverse_unif(lp->rng);

    if(bf->c1 == 1) {
        tw_rand_reverse_unif(lp->rng);
    }
}

void phold_finish(phold_state * s, tw_lp * lp) {
}

tw_lptype mylps[] = {
    {(init_f) phold_init,
     (event_f) phold_event_handler,
     (revent_f) phold_event_handler_rc,
     (final_f) phold_finish,
     (map_f) phold_map,
    sizeof(phold_state)},
    {0},
};

io_lptype iolps[] = {
    {(datatype_f) phold_datatype,
     (serialize_f) phold_serialize,
     (deserialize_f) phold_deserialize,
     sizeof(phold_state)},
     {0},
};

const tw_optdef app_opt[] = {
    TWOPT_GROUP("PHOLD Model"),
    TWOPT_STIME("remote", percent_remote, "desired remote event rate"),
    TWOPT_UINT("nlp", nlp_per_pe, "number of LPs per processor"),
    TWOPT_STIME("mean", mean, "exponential distribution mean for timestamps"),
    TWOPT_STIME("mult", mult, "multiplier for event memory allocation"),
    TWOPT_STIME("lookahead", lookahead, "lookahead for events"),
    TWOPT_UINT("start-events", g_phold_start_events, "number of initial messages per LP"),
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
    io_opts();
    tw_init(&argc, &argv);

    if( lookahead > 1.0 ) {
        tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");
    }

    //reset mean based on lookahead
    mean = mean - lookahead;

    g_tw_memory_nqueues = 16; // give at least 16 memory queue event

    offset_lpid = g_tw_mynode * nlp_per_pe;
    ttl_lps = tw_nnodes() * g_tw_npe * nlp_per_pe;
    g_tw_events_per_pe = (mult * nlp_per_pe * g_phold_start_events) + 
    optimistic_memory;
    //g_tw_rng_default = TW_FALSE;
    g_tw_lookahead = lookahead;

    tw_define_lps(nlp_per_pe, sizeof(phold_message), 0);

    g_tw_lp_types = mylps;
    g_io_lp_types = iolps;
    tw_lp_setup_types();

    if( g_tw_mynode == 0 ){
        printf("========================================\n");
        printf("PHOLD Model Configuration..............\n");
        printf("   Lookahead..............%lf\n", lookahead);
        printf("   Start-events...........%u\n", g_phold_start_events);
        printf("   stagger................%u\n", stagger);
        printf("   Mean...................%lf\n", mean);
        printf("   Mult...................%lf\n", mult);
        printf("   Memory.................%u\n", optimistic_memory);
        printf("   Remote.................%lf\n", percent_remote);
        printf("========================================\n\n");
    }

    // HACKY HACK HACK
    //tw_run();
    tw_pe *me;
    //late_sanity_check(); //static
    me = setup_pes(); //un-static'd

    //tw_sched_init
    tw_init_kps(me);
    tw_init_lps(me);

    tw_net_barrier(me);

    // this is when the simulation starts
    //coughcoughHACK!
    io_init(g_io_number_of_files, g_io_number_of_partitions);

    if (io_store != 0) {
        io_store_checkpoint("phold_checkpoint");
    } else {
        io_load_checkpoint("phold_checkpoint");
    }

    io_final();

    tw_end();

    return 0;
}
