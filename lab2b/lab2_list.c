// NAME: Christopher Aziz
// EMAIL: caziz@ucla.edu
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

#define KEY_SIZE 20

int opt_yield = 0;
int numthreads = 1;
int numiterations = 1;
int numlists = 1;
char opt_sync = NONE;
long long* mutex_wait_times;

typedef struct {
    SortedList_t *sorted_list;
    pthread_mutex_t mutex;
    int spin_lock;
} Sublist;

Sublist *sublists;
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

// hash element key to a number between 0 and numlists - 1
unsigned int hash(const char *key) {
    unsigned int hash = 0;
    for(int i = 0; key[i] != '\0'; i++)
        hash = key[i] + (hash << 6) + (hash << 16) - hash;
    return hash % numlists;
}

void parse_options(int argc, char **argv) {
    struct option longopts[6] =
    {
        {"threads",     required_argument, 0, 't'},
        {"iterations",  required_argument, 0, 'i'},
        {"yield",       required_argument, 0, 'y'},
        {"sync",        required_argument, 0, 's'},
        {"lists",       required_argument, 0, 'l'},
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
            case 'y':                   // yield flag
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
            case 's':                   // sync option
                if (strlen(optarg) != 1) {
                    fprintf(stderr, "Invalid sync argument\n");
                    fatal_usage();
                }
                switch(optarg[0]) {
                    case MUTEX:
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
            case 'l':                   // sublists num
                numlists = atoi(optarg);
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
        key[i] = (char) (rand() % 26) + 'a';
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
    element->key  = rand_key(KEY_SIZE);
    return element;
}

SortedListElement_t **create_unsorted_list(int size) {
    SortedListElement_t **unsorted_list = malloc(sizeof(SortedListElement_t*) * size);
    srand((unsigned int) time(NULL));
    for (int i = 0; i < size; i++)
        unsorted_list[i] = create_element();
    return unsorted_list;
}

void signal_handler(int signal) {
    if (signal == SIGSEGV)// invalid option
        fatal_error("Segmentation fault caught", 2);
}

long long time_diff_ns(struct timespec *start_ts, struct timespec *finish_ts) {
    return (finish_ts->tv_nsec - start_ts->tv_nsec) +
        ((finish_ts->tv_sec - start_ts->tv_sec) * 1000000000);
}

// synchronize depending on paramters
void lock(Sublist *sublist,
          struct timespec* start_ts,
          struct timespec* finish_ts,
          long long* mutex_wait_time) {
    switch (opt_sync) {
        case NONE:
            return;
        case MUTEX:
            clock_gettime(CLOCK_MONOTONIC, start_ts);
            pthread_mutex_lock(&sublist->mutex);
            clock_gettime(CLOCK_MONOTONIC, finish_ts);
            *mutex_wait_time += time_diff_ns(start_ts, finish_ts);
            return;
        case SPIN_LOCK:
            while (__sync_lock_test_and_set(&sublist->spin_lock, 1));
            return;
        default:
            return;
    }
}

// unsynchronize depending on parameters
void unlock(Sublist *sublist) {
    switch (opt_sync) {
        case NONE:
            return;
        case MUTEX:
            pthread_mutex_unlock(&sublist->mutex);
            return;
        case SPIN_LOCK:
            __sync_lock_release(&sublist->spin_lock);
            return;
        default:
            return;
    }
}

void run_thread(void* value) {
    int index = *((int *) value);
    int iter_index = index * numiterations;
    struct timespec start_ts, finish_ts;
    mutex_wait_times[index] = 0;
    
    // insert elements into list
    for(int i = iter_index; i < iter_index + numiterations; i++) {
        const char *key = unsorted_list[i]->key;
        Sublist *sublist = &sublists[hash(key)];
        lock(sublist, &start_ts, &finish_ts, &mutex_wait_times[index]);
        SortedList_insert(sublist->sorted_list, unsorted_list[i]);
        unlock(sublist);
    }
    // calculate length of list
    int length = 0;
    for (int i = 0; i < numlists; i++) {
        lock(&sublists[i], &start_ts, &finish_ts, &mutex_wait_times[index]);
        length += SortedList_length(sublists[i].sorted_list);
        unlock(&sublists[i]);
        if (length < 0)
            fprintf(stderr, "List length is less than zero\n");
    }
    // lookup and delete elements from list
    for(int i = iter_index; i < iter_index + numiterations; i++) {
        const char *key = unsorted_list[i]->key;
        Sublist *sublist = &sublists[hash(key)];
        lock(sublist, &start_ts, &finish_ts, &mutex_wait_times[index]);
        SortedListElement_t *element = SortedList_lookup(sublist->sorted_list, key);
        if (element == NULL) {
            fprintf(stderr, "List corrupted: Failed to find element with key \"%s\" after it was inserted\n", key);
            exit(2);
        }
        unlock(sublist);
        lock(sublist, &start_ts, &finish_ts, &mutex_wait_times[index]);
        SortedList_delete(element);
        unlock(sublist);
    }
}

int main(int argc, char **argv) {
    parse_options(argc, argv);
    if(signal(SIGSEGV, signal_handler) == SIG_ERR)
        fatal_error("Failed to set signal handler", 2);
    
    // create sublists
    sublists = malloc(sizeof(Sublist) * numlists);
    for(int i = 0; i < numlists; i++) {
        sublists[i].sorted_list = create_list();
        if (opt_sync == MUTEX) {
            if (pthread_mutex_init(&sublists[i].mutex, NULL) < 0)
                fatal_error("Failed to initialize mutex", 2);
        } else if (opt_sync == SPIN_LOCK)
            sublists[i].spin_lock = 0;
    }
    // create elements to insert and delete
    const int numelements = numthreads * numiterations;
    unsorted_list = create_unsorted_list(numelements);
    
    // set up multithreading
    pthread_t* tids = malloc(sizeof(pthread_t) * numthreads);
    int *indexes = malloc(sizeof(int) * numthreads);
    
    // set up timing
    mutex_wait_times = malloc(sizeof(long long) * numthreads);
    struct timespec start_ts, finish_ts;
    int ret;
    ret = clock_gettime(CLOCK_MONOTONIC, &start_ts);
    if (ret < 0)
        fatal_error("Failed to get start time", 1);

    // create threads, do work
    for(int i = 0; i < numthreads; i++) {
        indexes[i] = i;
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
    // end timer
    ret = clock_gettime(CLOCK_MONOTONIC, &finish_ts);
    if (ret < 0)
        fatal_error("Failed to get finish time", 1);
    
    // check for corruption
    for (int i = 0; i < numlists; i++)
        if (SortedList_length(sublists[i].sorted_list) != 0) {
            fprintf(stderr, "List corrupted: List length is not zero after removing all elements\n");
            exit(2);
        }
    
    // create output string
    long total_time_ns = time_diff_ns(&start_ts, &finish_ts);
    char test_name[32];
    sprintf(test_name, "list-%s%s%s%s-%s",
            opt_yield == 0 ? "none" : "",
            opt_yield & INSERT_YIELD ? "i" : "",
            opt_yield & DELETE_YIELD ? "d" : "",
            opt_yield & LOOKUP_YIELD ? "l" : "",
            opt_sync == NONE ? "none" :
            opt_sync == MUTEX ? "m" : "s");
    int numops = numthreads * numiterations * 3;
    long avg_time_ns = total_time_ns / numops;
    long long mutex_wait_time = 0;
    if (opt_sync == MUTEX)
        for (int i = 0; i < numthreads; i++)
            mutex_wait_time += mutex_wait_times[i];
    const int num_lock_ops = 4;
    long long avg_mutex_wait_time_ns = mutex_wait_time / num_lock_ops;
    printf("%s,%i,%i,%i,%i,%li,%li,%lld\n",
           test_name,
           numthreads,
           numiterations,
           numlists,
           numops,
           total_time_ns,
           avg_time_ns,
           avg_mutex_wait_time_ns);
    
    // free memory
    for(int i = 0; i < numiterations; i++) {
        free((void *) unsorted_list[i]->key);
        free(unsorted_list[i]);
    }
    free(unsorted_list);
    free(indexes);
    free(mutex_wait_times);
    for(int i = 0; i < numlists; i++)
        free(sublists[i].sorted_list);
    free(sublists);
    free(tids);
    exit(0);
}
