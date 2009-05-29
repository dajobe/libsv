/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * tsv.h - Header for libtsv
 *
 * Copyright (C) 2009, David Beckett http://www.dajobe.org/
 * 
 * This package is Free Software
 * 
 * It is licensed under the following three licenses as alternatives:
 *   1. GNU Lesser General Public License (LGPL) V2.1 or any newer version
 *   2. GNU General Public License (GPL) V2 or any newer version
 *   3. Apache License, V2.0 or any newer version
 * 
 * You may not use this file except in compliance with at least one of
 * the above three licenses.
 * 
 * See LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 */


typedef enum {
  TSV_STATUS_OK = 0,
  TSV_STATUS_FAILED,
  TSV_STATUS_NO_MEMORY
} tsv_status_t;
  

typedef struct tsv_s tsv;
typedef tsv_status_t (*tsv_fields_callback)(tsv *t, void *user_data, char** fields, size_t *widths, size_t count);

/* bit flags for tsv_init() */
#define TSV_FLAGS_SAVE_HEADER (1<<0)

tsv* tsv_init(void *user_data, tsv_fields_callback callback, int flags);
void tsv_free(tsv *t);

int tsv_get_line(tsv *t);
const char* tsv_get_header(tsv *t, unsigned int i, size_t *width_p);

tsv_status_t tsv_parse_chunk(tsv *t, char *buffer, size_t len);

