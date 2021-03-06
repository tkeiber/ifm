#include "ifm.h"
#include "lattice.h"
#include "measure.h"
#include "rng.h"
#include "spin.h"
#include "state.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if (! LOMEM)
#include "wolff.h"
#else
#include "wolff_lomem.h"
#endif

/* probability to add a site to the cluster at a given temperature T */
#define WOLFF_P_ADD(T)  (1. - exp(-2./(T)))

/* Monte Carlo step (one unit of time) */
void mc_step(lattice *l, double p, rng_state *rng, wolff *w, state *s)
{
    int i;

    for (i = 0; i < UPDATES_PER_STEP; i++)
        wolff_update(l, p, rng, w, s);
}

void thermalize(lattice *l, double p, int t_therm,
                rng_state *rng, wolff *w, state *s)
{
    int i;
    for (i = 0; i < t_therm; i++) mc_step(l, p, rng, w, s);
}

void measure(lattice *l, double p, int t_meas,
             rng_state *rng, wolff *w, state *s, meas *ms)
{
    int i;

    meas_reset(ms);

    for (i = 0; i < t_meas; i++)
    {
        mc_step(l, p, rng, w, s);
        meas_accum(ms, w, s);
    }

    meas_average(ms, t_meas);
}

int main(int argc, char *argv[])
{
    lattice l;
    state s;
    wolff w;
    meas ms;
    rng_state *rng;
    double T;
    int i, seed;

    seed = (argc > 1) ? atoi(argv[1]) : time(NULL);
    lattice_init(&l);

    state_alloc(&s);
    wolff_init(&w);
    for (i = 0; i < LT_N; i++) s.spin[i] = SPIN_UP;
    s.magnetization = LT_N;
#if (! LOMEM)
    w.energy = - 1ll * LT_N * LT_Z / 2;
#endif

    rng = RNG_ALLOC();
    RNG_SEED(rng, seed);

    printf("%9s %6s " \
           "%12s %12s " \
           "%12s %12s",
           "Temp", "Stage",
           "M2", "M4",
           "C", "C2");
#if (! LOMEM)
    printf(" " \
           "%12s %12s " \
           "%12s %12s",
           "E", "E2",
           "EM2", "E2M4");
#endif
    printf("\n");

    while (fscanf(stdin, "%lf", &T) != EOF)
    {
        double p = WOLFF_P_ADD(T);
        int dt = pow(2, LOG_TIME_THERM);

        thermalize(&l, p, dt, rng, &w, &s);

        for (i = LOG_TIME_THERM; i < LOG_TIME_MEAS; i++)
        {
            dt = pow(2, i);
            measure(&l, p, dt, rng, &w, &s, &ms);

            printf("%9g %6d " \
                   "%12g %12g " \
                   "%12g %12g",
                    T, i,
                    ms.v[0][M2],  ms.v[1][M2],
                    ms.v[0][C],   ms.v[1][C]);
#if (! LOMEM)
            printf(" " \
                   "%12g %12g " \
                   "%12g %12g",
                    ms.v[0][E],   ms.v[1][E],
                    ms.v[0][EM2], ms.v[1][EM2]);
#endif
            printf("\n");
            fflush(stdout);
        }
    }

    state_free(&s);
    wolff_free(&w);
    RNG_FREE(rng);

    return EXIT_SUCCESS;
}
