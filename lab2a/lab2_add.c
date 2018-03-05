// NAME: Christopher Aziz
// EMAIL: caziz@ucla.edu
//
// lab2_add.c
// Program that implements and tests a shared variable add function
//

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <time.h>
#include <pthread.h>
#include <string.h>

long long counter = 0;
int numthreads = 1;
int numiterations = 1;
int opt_yield = 0;

// sync types
#define NONE 'n'
#define MUTEX 'm'
#define SPIN_LOCK 's'
#define COMPARE_AND_SWAP 'c'

char sync_type = NONE;

int spin_lock = 0;
pthread_mutex_t mutex;

// invalid command-line parameter/argument
void fatal_usage() {
    fprintf(stderr, "Usage: lab2_add [--threads=threadnum] [--iterations=iterationnum] [--yield] [--sync=[smc]]\n");
    exit(1);
}

// system call error or other failure
void fatal_error(char *error_msg, int error_code) {
    perror(error_msg);
    exit(error_code);
}

void parse_options(int argc, char **argv) {
    
    struct option longopts[5] =
    {
        {"threads",     required_argument, 0, 't'},
        {"iterations",  required_argument, 0, 'i'},
        {"yield",       no_argument,       0, 'y'},
        {"sync",        required_argument, 0, 's'},
        {0,             0,                 0,  0 }
    };
    while (1) {
        int option = getopt_long(argc, argv, "", longopts, NULL);
        if (option == -1)               // end of options
            break;
        switch (option) {
            case 0:                     // getopt already set flag
                break;
            case 't':                   // tread num
                numthreads = atoi(optarg);
                break;
            case 'i':                   // iterations num
                numiterations = atoi(optarg);
                break;
            case 'y':
                opt_yield++;
                break;
            case 's':
                if (strlen(optarg) != 1) {
                    fprintf(stderr, "Invalid sync argument\n");
                    fatal_usage();
                }
                switch (*optarg) {
                    case 'm':
                        if (pthread_mutex_init(&mutex, NULL) < 0)
                            fatal_error("Failed to initialize mutex", 2);
                        /* FALLTHRU */
                    case 's':
                    case 'c':
                        sync_type = *optarg;
                        break;
                    default:
                        fprintf(stderr, "Invalid sync argument\n");
                        fatal_usage();
                }
                break;
            case '?':                   // invalid option
                /* FALLTHRU */
            default:
                fprintf(stderr, "Invalid option\n");
                fatal_usage();
        }
    }
    // TODO: check atoi for int
}

void add(long long *pointer, long long value) {
    long long sum = *pointer + value;
    if (opt_yield)
        sched_yield();
    *pointer = sum;
}

void add_sub_helper(int value) {
    for(int i = 0; i < numiterations; i++) {
        switch (sync_type) {
            case MUTEX:
                pthread_mutex_lock(&mutex);
                add(&counter, value);
                pthread_mutex_unlock(&mutex);
                break;
            case SPIN_LOCK:
                while(__sync_lock_test_and_set(&spin_lock, 1));
                add(&counter, value);
                __sync_lock_release(&spin_lock);
                break;
            case COMPARE_AND_SWAP:
                ;long long exp, sum;
                do {
                    exp = counter;
                    sum = exp + value;
                    if(opt_yield)
                        sched_yield();
                } while(__sync_val_compare_and_swap(&counter, exp, sum) != exp);
                break;
            case NONE:
            default:
                add(&counter, value);
                break;
        }
    }
}

void add_sub(void *ptr) {
    (void) (ptr); /* silence unused param warning */
    add_sub_helper(1);
    add_sub_helper(-1);
}



void run_threads() {
    pthread_t *tids = malloc(numthreads * sizeof(pthread_t));
    if (tids == NULL)   // TODO: remove?
        fatal_error("Unable to allocate memory for pthreads", 2);
    int i, ret;
    // create threads, do work
    for(i = 0; i < numthreads; i++) {
        ret = pthread_create(&tids[i], NULL, (void *) &add_sub, NULL);
        if (ret != 0) {
            free(tids);
            fatal_error("Failed to create pthreads", 2);
        }
    }
    // join threads
    for(i = 0; i < numthreads; i++) {
        ret = pthread_join(tids[i], NULL);
        if (ret != 0) {
            free(tids);
            fatal_error("Failed to join pthreads", 2);
        }
    }
    free(tids);
}

int main(int argc, char **argv) {
    parse_options(argc, argv);
    
    struct timespec start_ts, finish_ts;
    int ret;
    ret = clock_gettime(CLOCK_MONOTONIC, &start_ts);
    if (ret < 0)
        fatal_error("Failed to get start time", 1);
    
    run_threads();
    
    ret = clock_gettime(CLOCK_MONOTONIC, &finish_ts);
    if (ret < 0)
        fatal_error("Failed to get finish time", 1);
    
    char test_name[32];
    sprintf(test_name, "add-%s%s%c",
            opt_yield ? "yield-" : "",
            sync_type == NONE ? "none" : "",
            sync_type == NONE ? '\0' : sync_type);
    
    long total_time_ns = (finish_ts.tv_nsec - start_ts.tv_nsec) +
        1000000000 * (finish_ts.tv_sec - start_ts.tv_sec);

    int numops = numthreads * numiterations * 2;
    long avg_time_ns = total_time_ns / numops;

    printf("%s,%i,%i,%i,%li,%li,%lld\n",
           test_name,
           numthreads,
           numiterations,
           numops,        // TODO: overflow?
           total_time_ns,
           avg_time_ns,
           counter);
    exit(0);
}


