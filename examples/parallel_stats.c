/* file: parallel_stats.c
 *
 * Demonstrates thread-safe usage of WFDB library using _ctx variants.
 *
 * This program processes multiple WFDB records in parallel, with each record
 * analyzed by a separate thread using its own WFDB_Context. Each thread
 * computes basic statistics (min, max, mean) for all signals in its assigned
 * record.
 *
 * Usage:
 *   parallel_stats RECORD [RECORD ...]
 *
 * Example:
 *   parallel_stats 100s 100s 100s
 *
 * This example demonstrates:
 * - Creating independent WFDB_Context instances for thread safety
 * - Using _ctx variants of WFDB functions (isigopen_ctx, getvec_ctx, etc.)
 * - Proper context lifecycle management (new, use, free)
 * - Concurrent processing of multiple records without interference
 * - Thread synchronization and result aggregation
 *
 * Compile:
 *   gcc -o parallel_stats parallel_stats.c -lwfdb -lpthread
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <wfdb/wfdb.h>

/* Structure to pass arguments to each worker thread */
typedef struct {
    char *record_name;      /* Name of the record to process */
    int thread_id;          /* Thread identifier for display */
    int status;             /* Return status (0 = success) */
    long samples_read;      /* Number of samples processed */
    int nsig;               /* Number of signals in record */
    double *signal_means;   /* Mean values for each signal */
    WFDB_Sample *signal_mins;  /* Minimum values for each signal */
    WFDB_Sample *signal_maxs;  /* Maximum values for each signal */
} thread_args_t;

/* Worker function executed by each thread */
void *process_record(void *arg)
{
    thread_args_t *args = (thread_args_t *)arg;
    WFDB_Context *ctx = NULL;
    WFDB_Siginfo *siginfo = NULL;
    WFDB_Sample *sample_buffer = NULL;
    long total_samples = 0;
    int i;

    /* Create a new, independent WFDB context for this thread */
    ctx = wfdb_context_new();
    if (ctx == NULL) {
        fprintf(stderr, "[Thread %d] Failed to create WFDB context\n",
                args->thread_id);
        args->status = -1;
        return NULL;
    }

    printf("[Thread %d] Processing record: %s\n",
           args->thread_id, args->record_name);

    /* Allocate signal info array */
    siginfo = calloc(WFDB_MAXSIG, sizeof(WFDB_Siginfo));
    if (siginfo == NULL) {
        fprintf(stderr, "[Thread %d] Memory allocation failed\n",
                args->thread_id);
        args->status = -1;
        wfdb_context_free(ctx);
        return NULL;
    }

    /* Open the record using the thread-specific context */
    args->nsig = isigopen_ctx(ctx, args->record_name, siginfo, WFDB_MAXSIG);
    if (args->nsig <= 0) {
        fprintf(stderr, "[Thread %d] Failed to open record %s\n",
                args->thread_id, args->record_name);
        args->status = -1;
        free(siginfo);
        wfdb_context_free(ctx);
        return NULL;
    }

    printf("[Thread %d] Opened record with %d signals\n",
           args->thread_id, args->nsig);

    /* Allocate buffers for statistics */
    args->signal_means = calloc(args->nsig, sizeof(double));
    args->signal_mins = calloc(args->nsig, sizeof(WFDB_Sample));
    args->signal_maxs = calloc(args->nsig, sizeof(WFDB_Sample));
    sample_buffer = calloc(args->nsig, sizeof(WFDB_Sample));

    if (!args->signal_means || !args->signal_mins ||
        !args->signal_maxs || !sample_buffer) {
        fprintf(stderr, "[Thread %d] Memory allocation failed\n",
                args->thread_id);
        args->status = -1;
        goto cleanup;
    }

    /* Initialize min/max arrays */
    for (i = 0; i < args->nsig; i++) {
        args->signal_mins[i] = WFDB_Sample_MAX;
        args->signal_maxs[i] = WFDB_Sample_MIN;
    }

    /* Read and process all samples using _ctx variant */
    while (getvec_ctx(ctx, sample_buffer) == args->nsig) {
        for (i = 0; i < args->nsig; i++) {
            WFDB_Sample val = sample_buffer[i];

            /* Accumulate for mean calculation */
            args->signal_means[i] += val;

            /* Track min and max */
            if (val < args->signal_mins[i])
                args->signal_mins[i] = val;
            if (val > args->signal_maxs[i])
                args->signal_maxs[i] = val;
        }
        total_samples++;
    }

    /* Calculate means */
    if (total_samples > 0) {
        for (i = 0; i < args->nsig; i++) {
            args->signal_means[i] /= total_samples;
        }
    }

    args->samples_read = total_samples;
    args->status = 0;

    printf("[Thread %d] Processed %ld samples from record %s\n",
           args->thread_id, total_samples, args->record_name);

cleanup:
    free(sample_buffer);
    free(siginfo);

    /* Clean up context - this calls wfdbquit() internally */
    wfdb_context_free(ctx);

    return NULL;
}

/* Main program */
int main(int argc, char *argv[])
{
    pthread_t *threads = NULL;
    thread_args_t *thread_args = NULL;
    int num_records;
    int i, j;
    int all_success = 1;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s RECORD [RECORD ...]\n", argv[0]);
        fprintf(stderr, "\nProcesses multiple WFDB records in parallel.\n");
        fprintf(stderr, "Each record is analyzed by a separate thread with its own context.\n");
        fprintf(stderr, "\nExample: %s 100s 100s 100s\n", argv[0]);
        return 1;
    }

    num_records = argc - 1;

    printf("=== Parallel WFDB Statistics Demo ===\n");
    printf("Processing %d records using thread-safe _ctx API\n\n", num_records);

    /* Allocate thread structures */
    threads = calloc(num_records, sizeof(pthread_t));
    thread_args = calloc(num_records, sizeof(thread_args_t));

    if (!threads || !thread_args) {
        fprintf(stderr, "Memory allocation failed\n");
        free(threads);
        free(thread_args);
        return 1;
    }

    /* Create worker threads - each with its own context */
    for (i = 0; i < num_records; i++) {
        thread_args[i].record_name = argv[i + 1];
        thread_args[i].thread_id = i;
        thread_args[i].status = -1;

        if (pthread_create(&threads[i], NULL, process_record, &thread_args[i]) != 0) {
            fprintf(stderr, "Failed to create thread %d\n", i);
            all_success = 0;
            break;
        }
    }

    /* Wait for all threads to complete */
    for (i = 0; i < num_records; i++) {
        pthread_join(threads[i], NULL);
    }

    /* Display results */
    printf("\n=== Results ===\n\n");

    for (i = 0; i < num_records; i++) {
        thread_args_t *args = &thread_args[i];

        printf("Record: %s (Thread %d)\n", args->record_name, args->thread_id);

        if (args->status != 0) {
            printf("  Status: FAILED\n\n");
            all_success = 0;
            continue;
        }

        printf("  Samples: %ld\n", args->samples_read);
        printf("  Signals: %d\n", args->nsig);
        printf("\n  Signal Statistics:\n");

        for (j = 0; j < args->nsig; j++) {
            printf("    Signal %d: min=%d, max=%d, mean=%.2f\n",
                   j, args->signal_mins[j], args->signal_maxs[j],
                   args->signal_means[j]);
        }
        printf("\n");

        /* Free per-thread result buffers */
        free(args->signal_means);
        free(args->signal_mins);
        free(args->signal_maxs);
    }

    /* Cleanup */
    free(threads);
    free(thread_args);

    printf("=== Thread-Safe Processing Complete ===\n");
    printf("All threads operated independently without interference.\n");
    printf("Each thread used its own WFDB_Context via _ctx functions.\n");

    return all_success ? 0 : 1;
}
