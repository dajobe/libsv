/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * parse.c - Parsing
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
#include <ctype.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <sv.h>
#include "sv_internal.h"


/* Ensure internal buffer is big enough for len more bytes */
static sv_status_t
sv_ensure_line_buffer_size(sv *t, size_t len)
{
  char *nbuffer;
  size_t nsize;

  if(t->len + len < t->size)
    return SV_STATUS_OK;

  nsize = (len + t->len) << 1;

  nbuffer = (char*)malloc(nsize + 1);
  if(!nbuffer)
    return SV_STATUS_NO_MEMORY;

  if(t->len)
    memcpy(nbuffer, t->buffer, t->len);
  nbuffer[t->len] = '\0';

  if(t->buffer)
    free(t->buffer);

  t->buffer = nbuffer;
  t->size = nsize;

  return SV_STATUS_OK;
}


#if defined(SV_DEBUG) && SV_DEBUG > 1
static void
sv_dump_buffer(FILE* fh, const char* label, const char* buffer, size_t len)
{
  size_t mylen=len;

  fprintf(fh, "%s (%zu bytes) >>>", label, len);
  if(mylen > 100)
    mylen = 100;
  fwrite(buffer, 1, mylen, fh);
  if(mylen != len)
    fputs("...", fh);
  fputs("<<<\n", fh);
}
#endif


static sv_status_t
sv_parse_line(sv *t, char *line, size_t len,  unsigned int* field_count_p)
{
  unsigned int column;
  int field_width = 0;
  int field_offset = 0;
  char* current_field = NULL;
  char* p = NULL;
  char** fields = t->fields;
  size_t* fields_widths = t->fields_widths;
  sv_status_t status;
  int field_is_quoted = 0;

#if defined(SV_DEBUG) && SV_DEBUG > 1
  if(fields)
    sv_dump_buffer(stderr, "(sv_parse_line): Parsing line", line, len);
#endif

  status = sv_ensure_fields_buffer_size(t, len);
  if(status)
    return status;

  if(fields) {
    current_field = t->fields_buffer;
    p = current_field;

    if(!p)
      return SV_STATUS_OK;
  }

  for(column = 0; 1; column++) {
    int c = -1;
    int field_ended = 0;
    int expect_sep = 0;

    if(column == len) {
      field_ended = 1;
      goto do_last;
    }

    c = line[column];

    if(t->flags & SV_FLAGS_QUOTED_FIELDS) {
      if(c == t->quote_char) {
        if(!field_width && !field_is_quoted) {
          field_is_quoted = 1;
  #if defined(SV_DEBUG) && SV_DEBUG > 1
          fprintf(stderr, "Field is quoted\n");
  #endif
          continue;
        } else if((t->flags & SV_FLAGS_DOUBLE_QUOTE) &&
                  column < len && line[column+1] == t->quote_char) {
  #if defined(SV_DEBUG) && SV_DEBUG > 1
          fprintf(stderr, "Doubled quote %c absorbed\n", t->quote_char);
  #endif
          column++;
          /* skip repeated quote - so it just replaces ""... with " */
          goto skip;
        } else if(column == len-1 || line[column+1] == t->field_sep) {
  #if defined(SV_DEBUG) && SV_DEBUG > 1
          fprintf(stderr, "Field ended on quote + sep\n");
  #endif
          field_ended = 1;
          expect_sep = 1;
          goto do_last;
        }
      }
    }

    if(!field_is_quoted && c == t->field_sep) {
#if defined(SV_DEBUG) && SV_DEBUG > 1
      fprintf(stderr, "Field ended on sep\n");
#endif
      field_ended = 1;
    }

    do_last:
    if(field_ended) {
      if(p)
        *p++ = '\0';

      if(fields) {

        if(t->flags & SV_FLAGS_STRIP_WHITESPACE) {
          /* Remove whitespace around a field */
          while(field_width > 0 && isspace(current_field[0])) {
            current_field++;
            field_width--;
          }

          while(field_width > 0 && isspace(current_field[field_width - 1]))
            field_width--;

          current_field[field_width] = '\0';
        }

        if(expect_sep)
          column++;

      }

#if defined(SV_DEBUG) && SV_DEBUG > 1
      if(fields) {
        fprintf(stderr, "  Field %d: %s (%d)\n", (int)field_offset, current_field, (int)field_width);
      }
#endif
      if(fields)
        fields[field_offset] = current_field;
      if(fields_widths)
        fields_widths[field_offset] = field_width;

      /* end loop when out of columns */
      if(column == len)
        break;

      /* otherwise got a tab so reset for next field */
      field_width = 0;
      field_is_quoted = 0;

      field_offset++;
      current_field = p;

      continue;
    }

    skip:
    if(fields)
      *p++ = c;
    field_width++;
  }


  if(field_count_p)
    *field_count_p = field_offset + 1;

  return SV_STATUS_OK;
}


static sv_status_t
sv_parse_chunk_line(sv* t, size_t line_len, int has_nl)
{
  size_t move_len = line_len;
  sv_status_t status = SV_STATUS_OK;
  unsigned int fields_count = 0;

  if(!line_len)
    goto skip_line;

  if(t->line_callback) {
    char c = t->buffer[line_len];

    t->buffer[line_len] = '\0';
    status = t->line_callback(t, t->callback_user_data, t->buffer, line_len);
    t->buffer[line_len] = c;
    if(status != SV_STATUS_OK)
      return status;
  }

  if(!t->fields_count) {
    unsigned int nfields;

    /* First line in the file - calculate number of fields */
    status = sv_parse_line(t, t->buffer, line_len, &nfields);
    if(status)
      return status;

    /* initialise fields arrays for nfields */
    status = sv_init_fields(t, nfields);
    if(status)
      return status;
  }

  status = sv_parse_line(t, t->buffer, line_len, &fields_count);
  if(status)
    return status;

  if(fields_count != t->fields_count) {
    t->bad_records++;
    if(t->flags & SV_FLAGS_BAD_DATA_ERROR) {
#if defined(SV_DEBUG) && SV_DEBUG > 1
      fprintf(stderr, "Error in line %d: saw %d fields expected %d\n",
              t->line, fields_count, t->fields_count);
#endif
      status = SV_STATUS_LINE_FIELDS;
      return status;
    }
#if defined(SV_DEBUG) && SV_DEBUG > 1
    fprintf(stderr, "Ignoring line %d: saw %d fields expected %d\n",
            t->line, fields_count, t->fields_count);
#endif
    /* Otherwise skip the line */
    goto skip_line;
  }

  if(t->line == 1 && (t->flags & SV_FLAGS_SAVE_HEADER)) {
    /* first line and header: turn fields into headers */
    unsigned int i;

    for(i = 0; i < t->fields_count; i++) {
      char *s = (char*)malloc(t->fields_widths[i]+1);
      if(!s) {
        status = SV_STATUS_NO_MEMORY;
        break;
      }
      memcpy(s, t->fields[i], t->fields_widths[i]+1);
      t->headers[i] = s;
      t->headers_widths[i] = t->fields_widths[i];
    }

    if(status == SV_STATUS_OK && t->header_callback) {
      /* got header fields - return them to user */
      status = t->header_callback(t, t->callback_user_data, t->headers,
                                  t->headers_widths, t->fields_count);
    }
  } else {
    /* data */

    if(t->data_callback) {
      /* got data fields - return them to user */
      status = t->data_callback(t, t->callback_user_data, t->fields,
                                t->fields_widths, t->fields_count);
    }
  }

  skip_line:

  if(has_nl)
    move_len++;

  /* adjust buffer - remove 'line_len+1' bytes from start of buffer */
  t->len -= move_len;

  /* this is an overlapping move */
  memmove(t->buffer, &t->buffer[move_len], t->len);

  /* This is not needed: guaranteed above */
  /* t->buffer[t->len] = '\0' */

  t->line++;

  return status;
}


/*
 * sv_parse_chunk_internal:
 * @t: sv object
 * @buffer: buffer to parse (or NULL)
 * @len: length of @buffer (or 0)
 *
 * Internal - parse a chunk of data
 *
 * The input data is finished (EOF) if either @buffer is NULL or @len is 0
 *
 * Return value: #SV_STATUS_OK on success
 */
sv_status_t
sv_internal_parse_chunk(sv *t, char *buffer, size_t len)
{
  size_t offset = 0;
  sv_status_t status = SV_STATUS_OK;
  /* End of input if either of these is NULL */
  int is_end = (!buffer || !len);

  if(!is_end) {
    /* add new data to existing buffer */
    status = sv_ensure_line_buffer_size(t, len);
    if(status)
      return status;

    /* add new buffer */
    memcpy(t->buffer + t->len, buffer, len);

    /* always ensure it is NUL terminated even if input chunk was not */
    t->len += len;
    t->buffer[t->len] = '\0';
  }

  /* look for an end of line to do some work */
  for(offset = 0; offset < t->len; offset++) {
    char c = t->buffer[offset];

    /* skip \n when just seen \r - i.e. \r\n or CR LF */
    if(t->last_char == '\r' && c == '\n') {
#if defined(SV_DEBUG) && SV_DEBUG > 1
      fprintf(stderr, "Skipping a \\n after \\r\n");
#endif

      /* adjust buffer */
      t->len -= 1;

      /* this is an overlapping move */
      memmove(t->buffer, &t->buffer[1], t->len);

      t->last_char = '\0';
      continue;
    }

    if(c != '\r' && c != '\n')
      continue;

    t->last_char = c;

#if defined(SV_DEBUG) && SV_DEBUG > 1
    sv_dump_buffer(stderr, "Starting buffer", t->buffer, t->len);
#endif

    /* found a line */
    status = sv_parse_chunk_line(t, offset, 1);
    if(status != SV_STATUS_OK)
      break;

    offset = -1; /* so for loop starts at 0 */
  }

  if(is_end && status == SV_STATUS_OK) {
    /* If end of input and there is a non-empty buffer left, try to
     * parse it all as the last line.  It will NOT contain newlines.
     */
    if(t->len)
      status = sv_parse_chunk_line(t, t->len, 0);
  }

  return status;
}
