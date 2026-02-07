/* file: wfdb_context.c	2026
   WFDB library context management.
*/

#include "wfdb_context.h"
#include <stdlib.h>
#include <string.h>

/* The default global context, used by all legacy API functions. */
static WFDB_Context default_context;
static int default_context_initialized;

WFDB_Context *wfdb_get_default_context(void)
{
    if (!default_context_initialized) {
	memset(&default_context, 0, sizeof(default_context));
	default_context.initialized = 1;
	default_context.error_print = 1;
	default_context.wfdb_mem_behavior = 1;
#if WFDB_NETFILES
	default_context.nf_page_size = NF_PAGE_SIZE;
#endif
	default_context_initialized = 1;
    }
    return &default_context;
}

WFDB_Context *wfdb_context_new(void)
{
    WFDB_Context *ctx = calloc(1, sizeof(WFDB_Context));
    if (ctx) {
	ctx->initialized = 1;
	ctx->error_print = 1;
	ctx->wfdb_mem_behavior = 1;
#if WFDB_NETFILES
	ctx->nf_page_size = NF_PAGE_SIZE;
#endif
    }
    return ctx;
}

/* Public API: create a new context. */
WFDB_Context *wfdb_context_create(void)
{
    return wfdb_context_new();
}

/* Public API: destroy a context. */
void wfdb_context_destroy(WFDB_Context *ctx)
{
    wfdb_context_free(ctx);
}

void wfdb_context_free(WFDB_Context *ctx)
{
    if (ctx && ctx != &default_context) {
	/* Free calibration list */
	while (ctx->first_cle) {
	    struct cle *next = ctx->first_cle->next;
	    free(ctx->first_cle->sigtype);
	    free(ctx->first_cle->units);
	    free(ctx->first_cle);
	    ctx->first_cle = next;
	}
	/* Free annotation data */
	{
	    unsigned i;
	    for (i = 0; i < ctx->niaf; i++) {
		if (ctx->iad[i]) {
		    if (ctx->iad[i]->file)
			wfdb_fclose(ctx->iad[i]->file);
		    free(ctx->iad[i]->info.name);
		    free(ctx->iad[i]);
		}
	    }
	    free(ctx->iad);
	    for (i = 0; i < ctx->noaf; i++) {
		if (ctx->oad[i]) {
		    free(ctx->oad[i]->info.name);
		    free(ctx->oad[i]->rname);
		    free(ctx->oad[i]);
		}
	    }
	    free(ctx->oad);
	}
	/* Free I/O state */
	free(ctx->wfdbpath);
	free(ctx->wfdbpath_init);
	free(ctx->wfdb_filename);
	free(ctx->error_message);
	{
	    struct wfdb_path_component *c0 = NULL, *c1 = ctx->wfdb_path_list;
	    while (c1) {
		c0 = c1->next;
		free(c1->prefix);
		free(c1);
		c1 = c0;
	    }
	}
	/* Note: p_wfdb etc. are putenv strings - don't free for default ctx,
	   but safe to free for non-default contexts since they don't putenv */
	free(ctx->p_wfdb);
	free(ctx->p_wfdbcal);
	free(ctx->p_wfdbannsort);
	free(ctx->p_wfdbgvmode);
#if WFDB_NETFILES
	/* Clean up libcurl state */
	if (ctx->www_done_init) {
	    curl_easy_cleanup(ctx->curl_ua);
	    ctx->curl_ua = NULL;
	    curl_global_cleanup();
	    ctx->www_done_init = 0;
	}
	{
	    int i;
	    for (i = 0; ctx->www_passwords && ctx->www_passwords[i]; i++)
		free(ctx->www_passwords[i]);
	    free(ctx->www_passwords);
	}
	free(ctx->curl_ua_string);
#endif
	free(ctx);
    }
}
