
/*------------------------------------------------------------------------*/
/* Tareas con prioridades y planificador                                  */
/*------------------------------------------------------------------------*/
#include <stdlib.h>
#include <stddef.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <sched.h>
#include <pthread.h>
#include <sys/mman.h>
#define PRIORIDAD_A     24
#define ITER_A          40
#define INC_A           100
#define PRIORIDAD_B     26
#define ITER_B          40
#define INC_B           1
struct Data {
    pthread_mutex_t mutex;
    int cnt;
};
/*------------------------------------------------------------------------*/
/*
* Nota: los "printf" están con el objetivo de depuración, para que el
* alumno pueda analizar el comportamiento de las tareas
*/
/*------------------------------------------------------------------------*/
void espera_activa(time_t seg)
{
    volatile time_t t = time(0) + seg;
    while (time(0) < t) { /* esperar activamente */ }
}
/*------------------------------------------------------------------------*/
/*-- Tareas --------------------------------------------------------------*/
/*------------------------------------------------------------------------*/
void* tarea_a(void* arg)
{
    struct Data* data = (struct Data *)arg;
    struct sched_param param;
    const char* pol;
    int i, policy;

    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--";
    for (i = 0; i < ITER_A; ++i) {

        data->cnt += INC_A;
        printf("Tarea A [%s:%d]: %d\n",
        pol, param.sched_priority, data->cnt);

        espera_activa(1);
    }
    return NULL;
}
/*------------------------------------------------------------------------*/
void* tarea_b(void* arg)
{
    struct Data* data = (struct Data *)arg;
    struct sched_param param;
    const char* pol;
    int i, policy;

    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--";
    for (i = 0; i < ITER_B; ++i) {
   
        data->cnt += INC_B;
        printf("Tarea B [%s:%d]: %d\n",
        pol, param.sched_priority, data->cnt);

        espera_activa(1);
    }
    return NULL;
}
/*------------------------------------------------------------------------*/
/*-- Programa Principal --------------------------------------------------*/
/*------------------------------------------------------------------------*/
void usage(const char* nm)
{
    fprintf(stderr, "usage: %s [-h] [-ff] [-rr] [-p1] [-p2]\n", nm);
    exit(EXIT_FAILURE);
}
void get_args(int argc, const char* argv[],
int* policy, int* prio1, int* prio2){
    int i;
    if (argc < 2) {
        usage(argv[0]);
    } else {
        for (i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "-h")==0) {
                usage(argv[0]);
            } else if (strcmp(argv[i], "-ff")==0) {
                *policy = SCHED_FIFO;
            } else if (strcmp(argv[i], "-rr")==0) {
                *policy = SCHED_RR;
            } else if (strcmp(argv[i], "-p1")==0) {
                *prio1 = PRIORIDAD_A;
                *prio2 = PRIORIDAD_B;
            } else if (strcmp(argv[i], "-p2")==0) {
                *prio1 = PRIORIDAD_B;
                *prio2 = PRIORIDAD_A;
            } else {
                usage(argv[0]);
            }
        }
    }
}
int main(int argc, const char* argv[])
{
    struct Data shared_data;            // Structure to hold shared data and a mutex.
    pthread_attr_t attr;                // Thread attributes to configure scheduling policy and priority.
    struct sched_param param;           // Scheduling parameters, including priority.
    const char* pol;                    // String to hold the name of the scheduling policy.
    int pcy, policy = SCHED_FIFO;       // Scheduling policy; default is FIFO (First-In-First-Out).
    int prio0 = 1, prio1 = 1, prio2 = 1; // Priorities for the main thread and two child threads.
    pthread_t t1, t2;                   // Thread handles for the two child threads.

    /*
     * Determine the scheduling policy and priorities based on user input.
     * If not provided, defaults are used.
     */
    get_args(argc, argv, &policy, &prio1, &prio2);

    /*
     * Set the main thread's priority to the highest value
     * (one higher than the maximum of the two child thread priorities).
     */
    prio0 = (prio1 > prio2 ? prio1 : prio2) + 1;

    /*
     * Lock all current and future memory pages in RAM to prevent swapping,
     * ensuring better performance for real-time operations.
     */
    (mlockall(MCL_CURRENT | MCL_FUTURE));

    /*
     * Get the current scheduling parameters of the main thread.
     */
    (pthread_getschedparam(pthread_self(), &pcy, &param));

    /*
     * Set the priority of the main thread to the highest value (prio0).
     */
    param.sched_priority = prio0;
    (pthread_setschedparam(pthread_self(), policy, &param));

    /*
     * Determine the name of the scheduling policy for display purposes.
     */
    pol = (policy == SCHED_FIFO) ? "FF" : (policy == SCHED_RR) ? "RR" : "--";

    /*
     * Initialize the shared data structure and mutex.
     * This shared data will be accessed by both threads.
     */
    shared_data.cnt = 0;                // Initialize the shared counter to 0.
    (pthread_mutex_init(&shared_data.mutex, NULL)); // Initialize the mutex for shared data.

    /*
     * Initialize thread attributes and set them to use explicit scheduling.
     */
    (pthread_attr_init(&attr));
    (pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED));
    (pthread_attr_setschedpolicy(&attr, policy)); // Set the chosen scheduling policy.

    /*
     * Configure and create the first thread with priority `prio1`.
     */
    param.sched_priority = prio1;
    (pthread_attr_setschedparam(&attr, &param)); // Set the thread priority.
    (pthread_create(&t1, &attr, tarea_a, &shared_data)); // Create thread t1 to run `tarea_a`.

    /*
     * Configure and create the second thread with priority `prio2`.
     */
    param.sched_priority = prio2;
    (pthread_attr_setschedparam(&attr, &param)); // Set the thread priority.
    (pthread_create(&t2, &attr, tarea_b, &shared_data)); // Create thread t2 to run `tarea_b`.

    /*
     * Destroy the thread attributes object as it is no longer needed.
     */
    (pthread_attr_destroy(&attr));

    /*
     * Print the scheduling policy and priority of the main thread for debugging purposes.
     */
    printf("Tarea principal [%s:%d]\n", pol, prio0);

    /*
     * Wait for both threads (t1 and t2) to finish before continuing.
     */
    (pthread_join(t1, NULL)); // Wait for t1 to finish.
    (pthread_join(t2, NULL)); // Wait for t2 to finish.

    /*
     * Destroy the mutex as it is no longer needed.
     */
    (pthread_mutex_destroy(&shared_data.mutex));

    return 0; // Exit the program.
}

}
/*------------------------------------------------------------------------*/