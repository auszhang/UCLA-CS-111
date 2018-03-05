// NAME: Christopher Aziz
// EMAIL: caziz@ucla.edu
// ID: 304806012
//
// lab2_list.c
// Program that uses sorted list in a multithread environment
//

#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <pthread.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include "SortedList.h"

#define NONE 'n'
#define MUTEX 'm'
#define SPIN_LOCK 's'

int opt_yield = 0;
int numthreads = 1;
int numiterations = 1;
char opt_sync = NONE;
int spin_lock = 0;
pthread_mutex_t mutex;

SortedList_t *list;
SortedListElement_t **unsorted_list;

// invalid command-line parameter/argument
void fatal_usage() {
    fprintf(stderr, "Usage: lab2_add [--threads=threadnum] [--iterations=iterationnum] [--yield=[idl]]\n");
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
        {"yield",       required_argument, 0, 'y'},
        {"sync",        optional_argument, 0, 's'},
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
                for(int i = 0; i < (int) strlen(optarg); i++) {
                    switch (optarg[i]) {
                        case 'i':
                            opt_yield |= INSERT_YIELD;
                            break;
                        case 'd':
                            opt_yield |= DELETE_YIELD;
                            break;
                        case 'l':
                            opt_yield |= LOOKUP_YIELD;
                            break;
                        default:
                            fprintf(stderr, "Invalid yield argument\n");
                            fatal_usage();
                    }
                }
                break;
            case 's':
                if (strlen(optarg) != 1) {
                    fprintf(stderr, "Invalid sync argument\n");
                    fatal_usage();
                }
                switch(*optarg) {
                    case MUTEX:
                        if (pthread_mutex_init(&mutex, NULL) < 0)
                            fatal_error("Failed to initialize mutex", 2);
                        opt_sync = MUTEX;
                        break;
                    case SPIN_LOCK:
                        opt_sync = SPIN_LOCK;
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
}

char *rand_key(int size) {
    char *key = malloc(sizeof(char) * (size + 1));
    for (int i = 0; i < size; i++)
        key[i] = (char) rand() % 26 + 'a';
    key[size] = '\0';
    return key;
}

SortedList_t *create_list() {
    SortedList_t *head = malloc(sizeof(SortedList_t));
    head->prev = head;
    head->next = head;
    head->key = NULL;
    return head;
}

SortedListElement_t *create_element() {
    SortedListElement_t *element = malloc(sizeof(SortedListElement_t));
    element->next = NULL;
    element->prev = NULL;
    element->key  = rand_key(10);
    return element;
}

void populate_unsorted_list(SortedListElement_t **unsorted_list, int size) {
    srand((unsigned int) time(NULL));
    for (int i = 0; i < size; i++)
        unsorted_list[i] = create_element();
}

void signal_handler(int signal) {
    if (signal == SIGSEGV)// invalid option
        fatal_error("Segmentation fault caught", 2);
}

void sync() {
    switch (opt_sync) {
        case NONE:
            return;
        case MUTEX:
            pthread_mutex_lock(&mutex);
            return;
        case SPIN_LOCK:
            while (__sync_lock_test_and_set(&spin_lock, 1));
            return;
        default:
            return;
    }
}

void unsync() {
    switch (opt_sync) {
        case NONE:
            return;
        case MUTEX:
            pthread_mutex_unlock(&mutex);
            return;
        case SPIN_LOCK:
            __sync_lock_release(&spin_lock);
            return;
        default:
            return;
    }
}

void run_thread(void* value) {
    int index = *((int *) value);
    for(int i = index; i < index + numiterations; i++) {
        sync();
        SortedList_insert(list, unsorted_list[i]);
        unsync();
    }
    sync();
    int length = SortedList_length(list);
    (void) length;
    unsync();
    for(int i = index; i < index + numiterations; i++) {
        sync();
        SortedListElement_t *element = SortedList_lookup(list, unsorted_list[i]->key);
        unsync();
        if (element == NULL) {
            fprintf(stderr, "List corrupted: Failed to find element after it was inserted");
            exit(2);
        }
        sync();
        SortedList_delete(element);
        unsync();
    }
}

int main(int argc, char **argv) {
    parse_options(argc, argv);
    if(signal(SIGSEGV, signal_handler) == SIG_ERR)
        fatal_error("Failed to set signal handler", 2);
    list = create_list();
    int numelements = numthreads * numiterations;
    unsorted_list = malloc(sizeof(SortedListElement_t) * numelements);
    populate_unsorted_list(unsorted_list, numelements);
    pthread_t* tids = malloc((sizeof(pthread_t) * numthreads));
    int *indexes = malloc(sizeof(int) * numthreads);
    
    struct timespec start_ts, finish_ts;
    int ret;
    // get start time
    ret = clock_gettime(CLOCK_MONOTONIC, &start_ts);
    if (ret < 0)
        fatal_error("Failed to get start time", 1);
    
    // create threads, do work
    for(int i = 0; i < numthreads; i++) {
        indexes[i] = i * numiterations;
        ret = pthread_create(&tids[i], NULL, (void *) &run_thread, (void *) &indexes[i]);
        if (ret != 0) {
            free(tids);
            fatal_error("Failed to create pthreads", 2);
        }
    }
    
    // join threads
    for(int i = 0; i < numthreads; i++) {
        ret = pthread_join(tids[i], NULL);
        if (ret != 0) {
            free(tids);
            fatal_error("Failed to join pthreads", 2);
        }
    }
    
    ret = clock_gettime(CLOCK_MONOTONIC, &finish_ts);
    if (ret < 0)
        fatal_error("Failed to get finish time", 1);
    
    if (SortedList_length(list) != 0) {
        fprintf(stderr, "List corrupted: List length is not zero after removing all elements");
        exit(2);
    }
    
    // TODO: check runtime is zero
    
    long total_time_ns = (finish_ts.tv_nsec - start_ts.tv_nsec) +
    1000000000 * (finish_ts.tv_sec - start_ts.tv_sec);
    
    char test_name[32];
    sprintf(test_name, "list-%s%s%s%s-%s",
            opt_yield == 0 ? "none" : "",
            opt_yield & INSERT_YIELD ? "i" : "",
            opt_yield & DELETE_YIELD ? "d" : "",
            opt_yield & LOOKUP_YIELD ? "l" : "",
            opt_sync == NONE ? "none" : opt_sync == MUTEX ? "m" : "s");

    int numops = numthreads * numiterations * 3;
    long avg_time_ns = total_time_ns / numops;

    printf("%s,%i,%i,1,%i,%li,%li\n",
           test_name,
           numthreads,
           numiterations,
           numops,
           total_time_ns,
           avg_time_ns);
    
    // free memory
    for(int i = 0; i < numiterations; i++) {
        free((void *) unsorted_list[i]->key);
        free(unsorted_list[i]);
    }
    free(unsorted_list);
    free(indexes);
    free(list);
    free(tids);
    exit(0);
}

