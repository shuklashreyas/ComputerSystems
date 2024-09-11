/**
 * Threaded merge sort.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <pthread.h>

#include <unistd.h>

#include <assert.h>

#define tty_printf(...) (isatty(1) && isatty(0) ? printf(__VA_ARGS__) : 0)

#ifndef SHUSH
#define log(...) (fprintf(stderr, __VA_ARGS__))
#else 
#define log(...)
#endif

int current_thread_count = 0;
pthread_mutex_t thread_count_mutex = PTHREAD_MUTEX_INITIALIZER;


pthread_cond_t start_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t start_mutex = PTHREAD_MUTEX_INITIALIZER;
int started_threads = 0;
void *merge_sort_thread(void *arg);

typedef struct {
    long *nums;
    int from;
    int to;
    long *target;
} sort_args;

/** The number of threads to be used for sorting. Default: 1 */
int thread_count = 1;

/**
 * Compute the delta between the given timevals in seconds.
 */
double time_in_secs(const struct timeval *begin, const struct timeval *end) {
  long s = end->tv_sec - begin->tv_sec;
  long ms = end->tv_usec - begin->tv_usec;
  return s + ms * 1e-6;
}

/**
 * Print the given array of longs, an element per line.
 */
void print_long_array(const long *array, int count) {
  for (int i = 0; i < count; ++i) {
    printf("%ld\n", array[i]);
  }
}

/**
 * Merge two slices of nums into the corresponding portion of target.
 */
void merge(long nums[], int from, int mid, int to, long target[]) {
  int left = from;
  int right = mid;

  int i = from;
  for (; i < to && left < mid && right < to; i++) {
    if (nums[left] <= nums[right]) {
      target[i] = nums[left];
      left++;
    }
    else {
      target[i] = nums[right];
      right++;
    }
  }
  if (left < mid) {
    memmove(&target[i], &nums[left], (mid - left) * sizeof(long));
  }
  else if (right < to) {
    memmove(&target[i], &nums[right], (to - right) * sizeof(long));
  }

}


/**
 * Sort the given slice of nums into target.
 *
 * Warning: nums gets overwritten.
 */
void merge_sort_aux(long nums[], int from, int to, long target[]) {
    if (to - from <= 1) {
        return;
    }

    int mid = (from + to) / 2;

    pthread_mutex_lock(&thread_count_mutex);
    if (current_thread_count < thread_count) {
        current_thread_count += 2; // Two new threads will be created
        pthread_mutex_unlock(&thread_count_mutex);

        sort_args args1 = {nums, from, mid, target};
        sort_args args2 = {nums, mid, to, target};

        pthread_t thread1, thread2;
        pthread_create(&thread1, NULL, merge_sort_thread, &args1);
        pthread_create(&thread2, NULL, merge_sort_thread, &args2);

        pthread_mutex_lock(&start_mutex);
        while (started_threads < 2) {
            pthread_cond_wait(&start_cond, &start_mutex);
        }
        pthread_mutex_unlock(&start_mutex);

        pthread_join(thread1, NULL);
        pthread_join(thread2, NULL);

        pthread_mutex_lock(&thread_count_mutex);
        current_thread_count -= 2; // Two threads have terminated
        pthread_mutex_unlock(&thread_count_mutex);
    } else {
        pthread_mutex_unlock(&thread_count_mutex);
        merge_sort_aux(target, from, mid, nums);
        merge_sort_aux(target, mid, to, nums);
    }

    merge(nums, from, mid, to, target);
}

void *merge_sort_thread(void *arg) {
    pthread_mutex_lock(&start_mutex);
    started_threads++;
    pthread_cond_signal(&start_cond);
    pthread_mutex_unlock(&start_mutex);

    sort_args *args = (sort_args *)arg;
    merge_sort_aux(args->nums, args->from, args->to, args->target);

    pthread_mutex_lock(&thread_count_mutex);
    current_thread_count--;
    pthread_mutex_unlock(&thread_count_mutex);

    return NULL;
}

/**
 * Sort the given array and return the sorted version.
 *
 * The result is malloc'd so it is the caller's responsibility to free it.
 *
 * Warning: The source array gets overwritten.
 */
long *merge_sort(long nums[], int count) {
  long *result = calloc(count, sizeof(long));
  assert(result != NULL);

  memmove(result, nums, count * sizeof(long));

  merge_sort_aux(nums, 0, count, result);

  return result;
}

/**
 * Based on command line arguments, allocate and populate an input and a 
 * helper array.
 *
 * Returns the number of elements in the array.
 */
int allocate_load_array(int argc, char **argv, long **array) {
  assert(argc > 1);
  int count = atoi(argv[1]);

  *array = calloc(count, sizeof(long));
  assert(*array != NULL);

  long element;
  tty_printf("Enter %d elements, separated by whitespace\n", count);
  int i = 0;
  while (i < count && scanf("%ld", &element) != EOF)  {
    (*array)[i++] = element;
  }

  return count;
}

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "Usage: %s <n>\n", argv[0]);
    return 1;
  }

  struct timeval begin, end;

  // get the number of threads from the environment variable SORT_THREADS
  if (getenv("MSORT_THREADS") != NULL)
    thread_count = atoi(getenv("MSORT_THREADS"));

  log("Running with %d thread(s). Reading input.\n", thread_count);

  // Read the input
  gettimeofday(&begin, 0);
  long *array = NULL;
  int count = allocate_load_array(argc, argv, &array);
  gettimeofday(&end, 0);

  log("Array read in %f seconds, beginning sort.\n", 
      time_in_secs(&begin, &end));
 
  // Sort the array
  gettimeofday(&begin, 0);
  long *result = merge_sort(array, count);
  gettimeofday(&end, 0);
  
  log("Sorting completed in %f seconds.\n", time_in_secs(&begin, &end));

  // Print the result
  gettimeofday(&begin, 0);
  print_long_array(result, count);
  gettimeofday(&end, 0);
  
  log("Array printed in %f seconds.\n", time_in_secs(&begin, &end));

  free(array);
  free(result);

  return 0;
}
