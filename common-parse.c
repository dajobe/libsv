/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * common-parse.c - Common parsing code
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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <sv.h>
#include "sv_internal.h"


/* Free fields, field_widths, headers and header_widths all based on
 * number of fields
 */
void
sv_free_fields(sv *t)
{
  if(t->fields) {
    unsigned int i;

    for(i = 0; i < t->fields_count; i++)
      free(t->fields[i]);

    free(t->fields);
    t->fields = NULL;
  }

  if(t->fields_widths) {
    free(t->fields_widths);
    t->fields_widths = NULL;
  }

  t->fields_count = 0;
}

static void
sv_free_headers(sv *t)
{
  if(t->headers) {
    unsigned int i;

    for(i = 0; i < t->headers_count; i++)
      free(t->headers[i]);
    free(t->headers);
    t->headers = NULL;
  }

  if(t->headers_widths) {
    free(t->headers_widths);
    t->headers_widths = NULL;
  }
}

/* Create or expand fields,widths,headers arrays to at least size nfields
 */
sv_status_t
sv_init_fields(sv *t, int nfields)
{
  char** cp;
  size_t* sp;

  if(t->fields_count >= nfields)
    return SV_STATUS_OK;

  cp = (char**)malloc(sizeof(char*) * (nfields + 1));
  if(!cp)
    goto failed;
  if(t->fields_count > 0) {
    memcpy(cp, t->fields, sizeof(char*) * t->fields_count);
    free(t->fields);
  }
  t->fields = cp;

  sp = (size_t*)malloc(sizeof(size_t) * (nfields + 1));
  if(!sp)
    goto failed;
  if(t->fields_count > 0) {
    memcpy(sp, t->fields_widths, sizeof(size_t) * t->fields_count);
    free(t->fields_widths);
  }
  t->fields_widths = sp;

  t->fields_count = nfields;
  return SV_STATUS_OK;

  failed:
  sv_free_fields(t);
  return SV_STATUS_NO_MEMORY;
}


void
sv_internal_parse_reset(sv* t)
{
  sv_free_fields(t);
  sv_free_headers(t);

  if(t->fields_buffer) {
    free(t->fields_buffer);
    t->fields_buffer = NULL;
  }
  t->fields_buffer_size = 0;
  t->fields_buffer_len = 0;

#ifdef SV_PARSE_V2
#else
  if(t->buffer) {
    free(t->buffer);
    t->buffer = NULL;
  }
  t->size = 0;
  t->len = 0;
#endif
  /* Set initial state */
  t->line = 1;

  t->status = SV_STATUS_OK;

  t->bad_records = 0;

#ifdef SV_PARSE_V2
  t->escape_char = '\\';
  t->state = SV_STATE_START_FILE;
#else
  t->last_char = '\0';
#endif
}


/* Ensure fields buffer is big enough for len bytes total */
sv_status_t
sv_ensure_fields_buffer_size(sv *t, size_t len)
{
  char *nbuffer;
  size_t nsize;

  if(t->fields_buffer_len + len < t->fields_buffer_size)
    return SV_STATUS_OK;

  nsize = (len + t->fields_buffer_len) << 1;

#if defined(SV_DEBUG) && SV_DEBUG > 2
  fprintf(stderr, "%d: Growing buffer from %d to %d bytes\n",
          t->line, (int)t->fields_buffer_size, (int)nsize);
#endif

  nbuffer = (char*)malloc(nsize + 1);
  if(!nbuffer)
    return SV_STATUS_NO_MEMORY;

  if(t->fields_buffer_size) {
    memcpy(nbuffer, t->fields_buffer, t->fields_buffer_len);
    free(t->fields_buffer);
  }
  nbuffer[t->fields_buffer_len] = '\0';

  t->fields_buffer = nbuffer;
  t->fields_buffer_size = nsize;

  return SV_STATUS_OK;
}
