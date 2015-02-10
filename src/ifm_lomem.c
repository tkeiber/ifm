#include "ifm.h"
#include "lattice.h"
#include "rng.h"
#include "spin.h"
#include "state.h"
#include "wolff_lomem.h"
#include "measure_lomem.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/* probability to add a site to the cluster at a given temperature T */
#define WOLFF_P_ADD(T)  (1. - exp(-2./(T)))

/* Monte Carlo step (one unit of time) */
void mc_step(lattice *l, double p, rng *rand, wolff_lomem *w, state *s)
{
    int i;

    for (i = 0; i < UPDATES_PER_STEP; i++)
        wolff_update(l, p, rand, w, s);
}

void thermalize(lattice *l, double p, int t_therm,
                rng *rand, wolff_lomem *w, state *s)
{
    int i;
    for (i = 0; i < t_therm; i++) mc_step(l, p, rand, w, s);
}

void measure(lattice *l, double p, int t_meas,
             rng *rand, wolff_lomem *w, state *s, meas_lomem *ms)
{
    int i;

    meas_lomem_reset(ms);

    for (i = 0; i < t_meas; i++)
    {
        mc_step(l, p, rand, w, s);
        meas_lomem_accum(ms, w, s);
    }

    meas_lomem_average(ms, t_meas);
}

int main(int argc, char *argv[])
{
    lattice l;
    state s;
    wolff_lomem w;
    meas_lomem ms;
    rng *rand;
    double T;
    int i, seed;

    seed = (argc > 1) ? atoi(argv[1]) : time(NULL);
    lattice_init(&l);

    state_alloc(&s);
    wolff_init(&w);
    for (i = 0; i < LT_N; i++) s.spin[i] = SPIN_UP;
    s.magnetization = LT_N;

    rand = RNG_ALLOC();
    RNG_SEED(rand, seed);

    printf("%9s %6s " \
           "%12s %12s " \
           "%12s %12s\n",
           "Temp", "Stage",
           "M2", "M4",
           "C", "C2");

    while (fscanf(stdin, "%lf", &T) != EOF)
    {
        double p = WOLFF_P_ADD(T);
        int dt = pow(2, LOG_TIME_THERM);

        thermalize(&l, p, dt, rand, &w, &s);

        for (i = LOG_TIME_THERM; i < LOG_TIME_MEAS; i++)
        {
            dt = pow(2, i);
            measure(&l, p, dt, rand, &w, &s, &ms);

            printf("%9g %6d " \
                   "%12g %12g " \
                   "%12g %12g\n",
                    T, i,
                    ms.v[0][M2],  ms.v[1][M2],
                    ms.v[0][C],   ms.v[1][C]);

            fflush(stdout);
        }
    }

    state_free(&s);
    wolff_free(&w);
    RNG_FREE(rand);

    return EXIT_SUCCESS;
}
