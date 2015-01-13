/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * sv.c - Parse separated-values (CSV, TSV) files
 *
 * Copyright (C) 2009-2015, Dave Beckett http://www.dajobe.org/
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


#ifdef SV_CONFIG
#include <sv_config.h>
#endif

#include <string.h>
#include <stdarg.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <sv.h>
#include "sv_internal.h"

/**
 * sv_new:
 * @user_data: user data to use for callbacks
 * @header_callback: callback to receive headers (or NULL)
 * @data_callback: callback to receive data rows (or NULL)
 * @field_sep: field separator ',' or '\t'
 *
 * Constructor - create an SV object
 *
 * Return value: new SV object or NULL on failure.
 */
sv*
sv_new(void *user_data, sv_fields_callback header_callback,
       sv_fields_callback data_callback,
       char field_sep)
{
  sv *t;

  if(field_sep != '\t' && field_sep != ',')
    return NULL;
  
  t = (sv*)malloc(sizeof(*t));
  if(!t)
    return NULL;

  t->field_sep = field_sep;

  t->callback_user_data = user_data;
  t->header_callback = header_callback;
  t->data_callback = data_callback;

  sv_reset(t);

  return t;
}


/**
 * sv_reset:
 * @sv: SV object
 * Reset the SV object for new input
 *
 * This discards any existing parsing state.
 */
void
sv_reset(sv *t)
{

  if(t->headers_widths) {
    free(t->headers_widths);
    t->headers_widths = NULL;
  }

  if(t->headers) {
    unsigned int i;
    
    for(i = 0; i < t->fields_count; i++)
      free(t->headers[i]);
    free(t->headers);
    t->headers = NULL;
  }

  if(t->fields_buffer) {
    free(t->fields_buffer);
    t->fields_buffer = NULL;
  }

  if(t->fields_widths) {
    free(t->fields_widths);
    t->fields_widths = NULL;
  }

  if(t->fields) {
    free(t->fields);
    t->fields = NULL;
  }

  if(t->buffer) {
    free(t->buffer);
    t->buffer = NULL;
  }


  /* Set initial state */
  t->line = 1;

  t->buffer = NULL;
  t->size = 0;
  t->len = 0;

  t->fields_count = 0;
  t->fields = NULL;
  t->fields_widths = NULL;

  t->fields_buffer = NULL;
  t->fields_buffer_size = 0;

  t->headers = NULL;
  t->headers_widths = NULL;

  /* default flags */
  t->flags = SV_FLAGS_SAVE_HEADER | SV_FLAGS_QUOTED_FIELDS;

  t->status = SV_STATUS_OK;

  t->bad_records = 0;

  t->last_char = '\0';

  t->quote_char = '"';

  t->line_callback = NULL;

}


/**
 * sv_free:
 * @t: SV object
 *
 * Destructor: destroy an SV object
 *
 */
void
sv_free(sv *t)
{
  if(!t)
    return;

  sv_reset(t);

  free(t);
}



/**
 * sv_get_line:
 * @t: sv object
 *
 * Get current SV line number
 *
 * Return value: line number or <0 on failure
 */
int
sv_get_line(sv *t)
{
  if(!t)
    return -1;

  return t->line;
}


/**
 * sv_get_header:
 * @t: sv object
 * @i: header index 0
 * @width_p: pointer to store width (or NULL)
 *
 * Get an SV header with optional width
 *
 * Return value: shared pointer to header or NULL if out of range
 */
const char*
sv_get_header(sv *t, unsigned int i, size_t *width_p)
{
  if(!t || !t->headers || i > t->fields_count)
    return NULL;

  if(width_p)
    *width_p = t->headers_widths[i];
  
  return (const char*)t->headers[i];
}


/**
 * sv_parse_chunk:
 * @t: sv object
 * @buffer: buffer to parse (or NULL)
 * @len: length of @buffer (or 0)
 *
 * Parse a chunk of data
 *
 * The input data is finished (EOF) if either @buffer is NULL or @len is 0
 *
 * Return value: #SV_STATUS_OK on success
 */
sv_status_t
sv_parse_chunk(sv *t, char *buffer, size_t len)
{
  return sv_internal_parse_chunk(t, buffer, len);
}
