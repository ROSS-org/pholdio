#include "dpholdio.h"
#include <assert.h>
#include <stdlib.h>

int io_store = 0;

/* Arrange the N elements of ARRAY in random order.
   Only effective if N is much smaller than RAND_MAX;
   if this may not be the case, use a better random
   number generator. */
void shuffle(tw_rng_stream *g, int *array, size_t n)
{
    if (n > 1)
    {
        size_t i;
        for (i = 0; i < n - 1; i++)
        {
	  size_t j = tw_rand_integer(g, i, n);
          int t = array[j];
          array[j] = array[i];
          array[i] = t;
        }
    }
}

/**
 * Functions for RIO
 **/
void pholdio_serialize (phold_state *s, void *buffer, tw_lp *lp) {
    memcpy(buffer, s, sizeof(phold_state));
    return;
}

void pholdio_deserialize (phold_state *s, void * buffer, tw_lp *lp) {
    memcpy(s, buffer, sizeof(phold_state));
    return;
}

size_t pholdio_size (phold_state *s, tw_lp *lp) {
    return sizeof(phold_state);
}

tw_peid
phold_map(tw_lpid gid)
{
	return (tw_peid) gid / g_tw_nlp;
}

typedef struct {
  unsigned long delta_count;
  double delta_average;
  double state_changed;
} delta_info;

#define DI_SIZE DUMMY_SIZE

delta_info DI[DI_SIZE];
int order[DI_SIZE];

void
phold_init(phold_state * s, tw_lp * lp)
{
	int              i;

	for (i = 0; i < DI_SIZE; i++) {
	  DI[i].delta_count = 0;
	  DI[i].delta_average = 0.0;
	  DI[i].state_changed = 0.0;
	  order[i] = i;
	}

	if( stagger )
	  {
	    for (i = 0; i < g_phold_start_events; i++)
	      {
		tw_event_send(
			      tw_event_new(lp->gid,
					   tw_rand_exponential(lp->rng, mean) + lookahead + (tw_stime)(lp->gid % (unsigned int)g_tw_ts_end),
					   lp));
	      }
	  }
	else
	  {
	    for (i = 0; i < g_phold_start_events; i++)
	      {
		tw_event_send(
			      tw_event_new(lp->gid,
					   tw_rand_exponential(lp->rng, mean) + lookahead,
					   lp));
	      }
	  }
}

void
phold_pre_run(phold_state * s, tw_lp * lp)
{
    tw_lpid	 dest;

	if(tw_rand_unif(lp->rng) <= percent_remote)
	{
		dest = tw_rand_integer(lp->rng, 0, ttl_lps - 1);
	} else
	{
		dest = lp->gid;
	}

	if(dest >= (g_tw_nlp * tw_nnodes()))
		tw_error(TW_LOC, "bad dest");

	tw_event_send(tw_event_new(dest, tw_rand_exponential(lp->rng, mean) + lookahead, lp));
}

static unsigned long delta_count = 0;
static double delta_average = 0.0;



void
phold_event_handler(phold_state * s, tw_bf * bf, phold_message * m, tw_lp * lp)
{
  long delta_size;
  int i;
  int idx;
  unsigned count;
  tw_lpid	 dest;
  long start_count = lp->rng->count;

  // This should be the FIRST thing to do in your event handler
  if (g_tw_synchronization_protocol == OPTIMISTIC ||
      g_tw_synchronization_protocol == OPTIMISTIC_DEBUG) {
    // Only do this in OPTIMISTIC mode
    tw_snapshot(lp, lp->type->state_sz);
  }

  // Generate the number of items to change
  count = tw_rand_ulong(lp->rng, 1, DI_SIZE / 4);
  idx = count;

  // Shuffle the order
  shuffle(lp->rng, order, DI_SIZE - 2);

  // Change 'count' items
  for (i = 0; i < count; i++) {
    // Change the item located at index order[i]
    s->dummy_state[order[i]] = tw_rand_integer(lp->rng, 0, LONG_MAX);
  }

  if(tw_rand_unif(lp->rng) <= percent_remote)
    {
      dest = tw_rand_integer(lp->rng, 0, ttl_lps - 1);
      // Makes PHOLD non-deterministic across processors! Don't uncomment
      /* dest += offset_lpid; */
      /* if(dest >= ttl_lps) */
      /* 	dest -= ttl_lps; */
    } else
    {
      dest = lp->gid;
    }

  if(dest >= (g_tw_nlp * tw_nnodes()))
    tw_error(TW_LOC, "bad dest");

  tw_event_send(tw_event_new(dest, tw_rand_exponential(lp->rng, mean) + lookahead, lp));

  // This should be the LAST thing you do in your event handler
  // (Take care to cover all possible exits!)
  if (g_tw_synchronization_protocol == OPTIMISTIC ||
      g_tw_synchronization_protocol == OPTIMISTIC_DEBUG) {
    double temp = delta_average;
    // Only do this in OPTIMISTIC mode
    delta_size = tw_snapshot_delta(lp, lp->type->state_sz);
    // Update the average
    temp *= delta_count;
    temp += (double)delta_size / lp->type->state_sz;
    delta_count++;
    delta_average = temp / delta_count;
    m->rng_count = lp->rng->count - start_count;
    temp = DI[idx].delta_average;
    temp *= DI[idx].delta_count;
    temp += (double)delta_size / lp->type->state_sz;
    DI[idx].delta_count = DI[idx].delta_count + 1;
    DI[idx].delta_average = temp / DI[idx].delta_count;
    DI[idx].state_changed = (double)(count * sizeof(double)) / lp->type->state_sz;
  }
}

void
phold_event_handler_rc(phold_state * s, tw_bf * bf, phold_message * m, tw_lp * lp)
{
    // We don't need to check g_tw_synchronization_protocol here since if
    // this gets called, we must be in an OPTIMISTIC mode anyway
    long count = m->rng_count;
    // This should be the FIRST thing to do in your reverse event handler
    tw_snapshot_restore(lp, lp->type->state_sz, lp->pe->cur_event->delta_buddy, lp->pe->cur_event->delta_size);
    while (count--) {
        tw_rand_reverse_unif(lp->rng);
    }
}

void
phold_finish(phold_state * s, tw_lp * lp)
{
}

tw_lptype       mylps[] = {
	{(init_f) phold_init,
     /* (pre_run_f) phold_pre_run, */
     (pre_run_f) NULL,
	 (event_f) phold_event_handler,
	 (revent_f) phold_event_handler_rc,
	 (final_f) phold_finish,
	 (map_f) phold_map,
	sizeof(phold_state)},
	{0},
};

#ifdef USE_RIO
io_lptype iolps[] = {
    {(serialize_f) pholdio_serialize,
     (deserialize_f) pholdio_deserialize,
     (model_size_f) pholdio_size},
     {0},
};
#endif

const tw_optdef app_opt[] =
{
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

int
main(int argc, char **argv, char **env)
{
	int		 i;

        // get rid of error if compiled w/ MEMORY queues
        g_tw_memory_nqueues=1;
	// set a min lookahead of 1.0
	lookahead = 1.0;
	tw_opt_add(app_opt);
	tw_init(&argc, &argv);

	if( lookahead > 1.0 )
	  tw_error(TW_LOC, "Lookahead > 1.0 .. needs to be less\n");

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
#ifdef USE_RIO
    g_io_lp_types = iolps;
#endif
    tw_lp_setup_types();

	if( g_tw_mynode == 0 ) {
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

	tw_clock start;

    // IF WE ARE USING RIO
	if (io_store != 2) {
#ifdef USE_RIO
    start = tw_clock_read();
		g_io_events_buffered_per_rank = 2*g_tw_nlp*g_phold_start_events;  // events past end time to store
    io_init();
    g_tw_pe[0]->stats.s_rio_load += (tw_clock_read() - start);
#endif
    }

    // IF WE ARE LOADING A CHECKPOINT
    if (io_store == 0) {
#ifdef USE_RIO
        io_load_checkpoint("dpholdio_checkpoint", INIT);
#endif
    }

    tw_run();

    // IF WE ARE STORING A CHECKPOINT
    if (io_store == 1) {
#ifdef USE_RIO
    	start = tw_clock_read();
      io_register_model_version(MODEL_VERSION);
      int ranks_per_file = tw_nnodes() / g_io_number_of_files;
      int data_file = g_tw_mynode / ranks_per_file;
      io_store_checkpoint("dpholdio_checkpoint", data_file);
      g_tw_pe[0]->stats.s_rio_load += (tw_clock_read() - start);
#endif
    }

	tw_end();

	if (g_tw_mynode == 0) {
	  printf("delta_average is %lf over %lu allocations\n", delta_average, delta_count);

	  for (i = 0; i < DI_SIZE; i++) {
	    if (DI[i].delta_count == 0) continue;
	    //printf("%d is %lf over %lu allocations\n", i, DI[i].delta_average, DI[i].delta_count);
	    printf("%lf changed %lf\n", DI[i].state_changed, DI[i].delta_average);
	  }
	}

	return 0;
}
