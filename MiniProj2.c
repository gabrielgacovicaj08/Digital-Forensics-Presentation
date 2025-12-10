/*
-------------------------------------------------------------
CMPS 4103 – Introduction to Operating Systems
Mini Project #2 
Author: Gabriel Gacovicaj
Environment: Ubuntu 
-------------------------------------------------------------
Problem Description:
  - Create a global 3D array A[100][100][20].
  - Each element is initialized as A[i][j][k] = i + j + k.
  - Create 5 threads (IDs 0–4). Each thread sums one 20-row section
    of A into a shared global variable SUM, one element at a time.
  - Run with semaphores to avoid race conditions when updating SUM.
  - Print the final SUM after all threads complete.

-------------------------------------------------------------
*/

#include <stdio.h>       // for printf()
#include <stdlib.h>      // for general utilities
#include <pthread.h>     // for POSIX threads
#include <semaphore.h>   // for POSIX semaphores

// -----------------------------------------------------------
// Global constants defining array size and number of threads
// -----------------------------------------------------------
#define X 100             // first dimension 
#define Y 100             // second dimension 
#define Z 20              // third dimension 
#define NUM_THREADS 5     // number of threads (5 sections of 20 rows each)


// Global shared resources (visible to all threads)
int A[X][Y][Z];           // shared 3D array
long long SUM = 0;        // shared accumulator variable 
sem_t mutex;              // semaphore (binary semaphore used as a mutex lock)

// Function prototype for the thread function
void* sum_section(void* arg);

// MAIN FUNCTION
int main() {
    pthread_t threads[NUM_THREADS];  // array of thread handles (IDs)
    int thread_ids[NUM_THREADS];     // array of integer IDs for each thread

    // Triple nested loop fills the array according to A[i][j][k] = i + j + k
    // This is done once in the main thread before spawning any worker threads.
    for (int i = 0; i < X; i++) {
        for (int j = 0; j < Y; j++) {
            for (int k = 0; k < Z; k++) {
                A[i][j][k] = i + j + k;
            }
        }
    }

    // sem_init(&sem, pshared, value)
    //   - &mutex: address of semaphore object
    //   - 0: means semaphore is shared only between threads in this process
    //   - 1: initial value = unlocked (binary semaphore)
    sem_init(&mutex, 0, 1);


    // Each thread receives a unique ID (0–4), which determines
    // which portion of the 3D array it will sum.
    for (int t = 0; t < NUM_THREADS; t++) {
        thread_ids[t] = t;  

        // pthread_create(pthread_t *thread, const pthread_attr_t *attr,
        //                void *(*start_routine)(void *), void *arg);
        // - Creates a new thread that starts executing sum_section().
        // - Passes a pointer to thread_ids[t] so the thread knows its ID.
        if (pthread_create(&threads[t], NULL, sum_section, &thread_ids[t]) != 0) {
            perror("Failed to create thread");
            exit(1);
        }
    }

    // The main thread will block (pause) until each worker thread terminates.
    // This ensures all calculations are finished before printing the SUM.
    for (int t = 0; t < NUM_THREADS; t++) {
        pthread_join(threads[t], NULL);
    }

    sem_destroy(&mutex);

    printf("Final SUM = %lld\n", SUM);

    return 0;  
}


// THREAD FUNCTION
// Each thread runs this function concurrently.
// The function receives its thread ID via 'arg' (void pointer).
// Based on the ID, it determines its row range and adds all
// elements in that slice to the global SUM.

void* sum_section(void* arg) {
    int tid = *(int*)arg;      // Convert void* back to int to get thread ID
    int start = tid * 20;      // Each thread handles 20 rows (0–19, 20–39, etc.)
    int end = (tid + 1) * 20 - 1;

    printf("Thread %d summing rows %d to %d...\n", tid, start, end);

    // Nested loops traverse the assigned slice of the array.
    // Each value is added to the global SUM variable.
    for (int i = start; i <= end; i++) {
        for (int j = 0; j < Y; j++) {
            for (int k = 0; k < Z; k++) {

                // -------------------------------------------------------
                // CRITICAL SECTION — only one thread may execute this at once
                // -------------------------------------------------------
                // sem_wait(&mutex)
                //   - If semaphore value > 0: decrements it and continues.
                //   - If semaphore value == 0: thread blocks until another
                //     thread calls sem_post(&mutex).
                sem_wait(&mutex);

                // Perform the actual shared resource update.
                // Only one thread at a time can modify SUM safely.
                SUM = SUM + A[i][j][k];

                // sem_post(&mutex)
                //   - Increments semaphore value.
                //   - If other threads are waiting, wakes one of them.
                sem_post(&mutex);
            }
        }
    }


    // Thread finishes its work.
    // pthread_exit(NULL) terminates the thread cleanly and
    // allows pthread_join() to detect its completion.
    pthread_exit(NULL);
}
