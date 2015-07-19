/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * read.c - Read SV
 *
 * Copyright (C) 2015, Dave Beckett http://www.dajobe.org/
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
#include <ctype.h>

#include <sv.h>
#include "sv_internal.h"


/* Free fields, field_widths, headers and header_widths all based on
 * number of fields
 */
static void
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
static sv_status_t
sv_init_fields(sv *t, unsigned int nfields)
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



static void
sv_reset_line_buffer(sv *t)
{
  t->len = 0;
}


void
sv_internal_free_line_buffer(sv *t)
{
  if(t->buffer) {
    free(t->buffer);
    t->buffer = NULL;
  }
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

  sv_reset_line_buffer(t);

  /* Set initial state */
  t->status = SV_STATUS_OK;

  t->state = SV_STATE_START_PARSE;
}


/* Ensure fields buffer is big enough for len bytes total */
static sv_status_t
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


/* Ensure line buffer is big enough for len more bytes */
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
sv_dump_buffer(FILE* fh, const char* buffer, size_t len)
{
  size_t mylen = len;
  size_t i;

  fputs(">>>", fh);
  if(mylen > 100)
    mylen = 100;
  for(i = 0; i < mylen; i++) {
    const unsigned char c = (const unsigned char)buffer[i];
    if(isprint(c))
      fputc(c, fh);
    else
      fprintf(fh, "\\x%02X", c);
  }
  if(mylen != len)
    fputs("...", fh);
  fprintf(fh, "<<< (%zu bytes)\n", len);
}
#endif

/**
 * sv_line_buffer_add_char:
 * @t: sv object
 * @c: char
 *
 * INTERNAL - Add char c to line buffer
 *
 * Return value: non-0 on failure
 */
static sv_status_t
sv_line_buffer_add_char(sv* t, char c)
{
  sv_status_t status;
  size_t llen = t->len;

  status = sv_ensure_line_buffer_size(t, t->len + 1);
  if(status)
    return status;

  t->buffer[llen++] = c;
  t->buffer[llen] = '\0';
  t->len = llen;

  return SV_STATUS_OK;
}


/**
 * sv_parse_save_cell:
 * @t: sv object
 *
 * INTERNAL - Save current cell to row
 *
 * Return value: non-0 on failure
 */
static sv_status_t
sv_parse_save_cell(sv* t)
{
  sv_status_t status;
  char* s;
  unsigned int cell_ix = t->fields_count;
  size_t cell_len = t->fields_buffer_len;

  status = sv_init_fields(t, cell_ix + 1);
  if(status)
    return status;

  s = (char*)malloc(cell_len + 1);
  if(!s)
    return SV_STATUS_NO_MEMORY;

  if(t->flags & SV_FLAGS_STRIP_WHITESPACE) {
    char *f = t->fields_buffer;
    /* Remove whitespace around a field */
    while(cell_len > 0 && isspace(*f)) {
      f++;
      cell_len--;
    }

    while(cell_len > 0 && isspace(f[cell_len - 1]))
      cell_len--;
    memcpy(s, f, cell_len);
  } else
    memcpy(s, t->fields_buffer, cell_len);
  s[cell_len] = '\0';

  t->fields[cell_ix] = s;
  t->fields_widths[cell_ix] = cell_len;

#if defined(SV_DEBUG) && SV_DEBUG > 2
  fprintf(stderr, "Line %d Cell %d ", t->line, cell_ix);
  sv_dump_buffer(stderr, s, cell_len);
#endif

  t->fields_buffer_len = 0;

  return SV_STATUS_OK;
}


/**
 * sv_parse_cell_add_char:
 * @t: sv object
 * @c: char
 *
 * INTERNAL - Add char c to current cell
 *
 * Return value: non-0 on failure
 */
static sv_status_t
sv_parse_cell_add_char(sv* t, char c)
{
  sv_status_t status;
  size_t len = t->fields_buffer_len;

  status = sv_ensure_fields_buffer_size(t, len + 1);
  if(status)
    return status;

  t->fields_buffer[len++] = c;
  t->fields_buffer_len = len;

  return SV_STATUS_OK;
}


/**
 * sv_parse_generate_row:
 * @t: sv object
 *
 * INTERNAL - generate a row from parse state
 *
 * Return value: non-0 on failure
 */
static sv_status_t
sv_parse_generate_row(sv *t)
{
  sv_status_t status = SV_STATUS_OK;

  if(t->skip_rows_remaining > 0) {
    t->skip_rows_remaining--;
#if defined(SV_DEBUG) && SV_DEBUG > 2
    fprintf(stderr, "Skipping row (%d remaining to skip) ",
            t->skip_rows_remaining);
    sv_dump_buffer(stderr, t->buffer, t->len);
#endif
    return status;
  }

  if(t->comment_prefix && !strncmp(t->buffer, t->comment_prefix,
                                   t->comment_prefix_len)) {
    char* comment = t->buffer + t->comment_prefix_len;
    size_t comment_len = t->len - t->comment_prefix_len;
#if defined(SV_DEBUG) && SV_DEBUG > 2
    fprintf(stderr, "Skipping comment row %d ", t->line);
    sv_dump_buffer(stderr, comment, comment_len);
#endif
    if(t->comment_callback)
      status = t->comment_callback(t, t->callback_user_data,
                                   comment, comment_len);
    return status;
  }

#if defined(SV_DEBUG) && SV_DEBUG > 2
  fprintf(stderr, "Generating row %d\n", t->line);
#endif

  if(t->line == 1 && (t->flags & SV_FLAGS_SAVE_HEADER)) {
    int nheaders = t->fields_count;
    char** cp;
    size_t* sp;
    int i;

    /* first line and header: turn fields into headers */
    cp = (char**)malloc(sizeof(char*) * (nheaders + 1));
    if(!cp)
      return SV_STATUS_NO_MEMORY;
    t->headers = cp;

    sp = (size_t*)malloc(sizeof(size_t) * (nheaders + 1));
    if(!sp)
      return SV_STATUS_NO_MEMORY;
    t->headers_widths = sp;

    for(i = 0; i < nheaders; i++) {
      int header_width = t->fields_widths[i];
      t->headers[i] = (char*)malloc(header_width + 1);
      if(!t->headers[i])
        return SV_STATUS_NO_MEMORY;

      memcpy(t->headers[i], t->fields[i], header_width + 1);
      t->headers_widths[i] = header_width;
    }
    t->headers_count = nheaders;
  }

  if(t->line_callback) {
    status = t->line_callback(t, t->callback_user_data,
                              t->buffer, t->len);
    if(status != SV_STATUS_OK)
      return status;
  }

  if(t->line == 1 && (t->flags & SV_FLAGS_SAVE_HEADER)) {
    if(t->header_callback) {
      /* got header fields - return them to user */
      status = t->header_callback(t, t->callback_user_data, t->headers,
                                  t->headers_widths, t->headers_count);
    }
  } else {
    /* data */

    if(t->data_callback) {
      /* got data fields - return them to user */
      status = t->data_callback(t, t->callback_user_data, t->fields,
                                t->fields_widths, t->fields_count);
    }
  }

  t->line++;

  return status;
}


/**
 * sv_parse_prepare_for_new_row:
 * @t: sv object
 *
 * INTERNAL - prepare for creating a new row
 *
 * Return value: non-0 on failure
 */
static void
sv_parse_prepare_for_new_row(sv *t)
{
  sv_free_fields(t);
  sv_reset_line_buffer(t);
}


#if defined(SV_DEBUG) && SV_DEBUG > 2
static const char* const sv_state_labels[SV_STATE_LAST + 1] = {
  "UNKNOWN",
  "START_PARSE",
  "START_FILE",
  "START_ROW",
  "EOL",
  "ESC_EOL",
  "COMMENT",
  "START_CELL",
  "IN_CELL",
  "ESC_IN_CELL",
  "IN_QUOTED_CELL",
  "ESC_IN_QUOTED_CELL",
  "QUOTE_IN_QUOTED_CELL"
};

const char*
sv_get_state_label(sv_parse_state state) {
  return (state <= SV_STATE_LAST) ? sv_state_labels[state] : NULL;
}

#endif

/**
 * sv_internal_parse_process_char:
 * @t: sv object
 * @c: char
 *
 * INTERNAL - process one character; NUL indicates end of input
 *
 * Return value: non-0 on failure
 */

static sv_status_t
sv_internal_parse_process_char(sv *t, char c)
{
  sv_status_t status;

#if defined(SV_DEBUG) && SV_DEBUG > 2
  if(isprint(c))
    fprintf(stderr, "State <%s> (%d) with char '%c' (0x%02X)\n",
            sv_get_state_label((int)t->state), (int)t->state,
            c, c);
  else
    fprintf(stderr, "State <%s> (%d) with char 0x%02X\n",
            sv_get_state_label((int)t->state), (int)t->state, c);
#endif

  redo:
  switch(t->state) {
    case SV_STATE_START_PARSE:
      /* once-only per parse initialising; may be altered by options */
      t->line = 1;
      t->skip_rows_remaining = t->skip_rows;
      t->bad_records = 0;

      t->state = SV_STATE_START_FILE;
      /* FALLTHROUGH */

    case SV_STATE_START_FILE:
      t->state = SV_STATE_START_ROW;
      /* FIXME - add BOM checking for encoding */

      /* FALLTHROUGH */
    case SV_STATE_START_ROW:
      if(!c)
        break;
      else if(c == '\n' || c == '\r') {
        t->state = SV_STATE_EOL;
        break;
      }
      t->state = SV_STATE_START_CELL;

      /* FALLTHROUGH */

    case SV_STATE_START_CELL:
      if(c == '\n' || c == '\r' || !c) {
        /* empty cell and end of row */
        status = sv_parse_save_cell(t);
        if(status)
          return status;

        sv_parse_generate_row(t);
        sv_parse_prepare_for_new_row(t);
        t->state = (!c ? SV_STATE_START_ROW : SV_STATE_EOL);
      } else if(t->quote_char && c == t->quote_char) {
        t->state = SV_STATE_IN_QUOTED_CELL;
      } else if(t->escape_char && c == t->escape_char) {
        t->state = SV_STATE_ESC_IN_CELL;
      } else if((c == ' ' || c == '\t') &&
                (t->flags & SV_FLAGS_STRIP_WHITESPACE))
        /* ignore whitespace at start of cell */
        ;
      else if(c == t->field_sep) {
        /* empty cell */
        status = sv_parse_save_cell(t);
        if(status)
          return status;
      } else {
        /* begin new unquoted cell */
        status = sv_parse_cell_add_char(t, c);
        if(status)
          return status;

        t->state = SV_STATE_IN_CELL;
      }
      break;

    case SV_STATE_ESC_IN_CELL:
      if(c == '\n' || c=='\r') {
        status = sv_parse_cell_add_char(t, c);
        if(status)
          return status;

        t->state = SV_STATE_ESC_EOL;
        break;
      }

      /* At end of input, add the missing EOL */
      if(!c)
        c = '\n';
      status = sv_parse_cell_add_char(t, c);
      if(status)
        return status;

      t->state = SV_STATE_IN_CELL;
      break;

    case SV_STATE_ESC_EOL:
      if(!c)
        break;

      /* FALLTHROUGH */

    case SV_STATE_IN_CELL:
      /* regular unquoted cell */
      if(c == '\n' || c == '\r' || !c) {
        /* end of line - return row */
        status = sv_parse_save_cell(t);
        if(status)
          return status;

        sv_parse_generate_row(t);
        sv_parse_prepare_for_new_row(t);
        t->state = (!c ? SV_STATE_START_ROW : SV_STATE_EOL);
      } else if(t->escape_char && c == t->escape_char) {
        t->state = SV_STATE_ESC_IN_CELL;
      } else if(c == t->field_sep) {
        status = sv_parse_save_cell(t);
        if(status)
          return status;

        t->state = SV_STATE_START_CELL;
      } else {
        status = sv_parse_cell_add_char(t, c);
        if(status)
          return status;
      }
      break;

    case SV_STATE_IN_QUOTED_CELL:
      if(!c)
        /* end of input */
        ;
      else if(t->escape_char && c == t->escape_char) {
        t->state = SV_STATE_ESC_IN_QUOTED_CELL;
      } else if(t->quote_char && c == t->quote_char) {
        if(t->flags & SV_FLAGS_DOUBLE_QUOTE)
          t->state = SV_STATE_QUOTE_IN_QUOTED_CELL;
        else
          t->state = SV_STATE_IN_CELL;
      } else {
        status = sv_parse_cell_add_char(t, c);
        if(status)
          return status;
      }
      break;

    case SV_STATE_ESC_IN_QUOTED_CELL:
      if(!c)
        /* end of input - add newline */
        c = '\n';
      status = sv_parse_cell_add_char(t, c);
      if(status)
        return status;

      t->state = SV_STATE_IN_QUOTED_CELL;
      break;

    case SV_STATE_QUOTE_IN_QUOTED_CELL:
      /* after a quote char in an quoted cell */
      if(t->quote_char && c == t->quote_char) {
        /* <quote><quote> so write just 1 */
        status = sv_parse_cell_add_char(t, c);
        if(status)
          return status;

        t->state = SV_STATE_IN_QUOTED_CELL;
      } else if(c == t->field_sep) {
        /* had <quote><sep> so cell has ended - save it and start new one */
        status = sv_parse_save_cell(t);
        if(status)
          return status;

        t->state = SV_STATE_START_CELL;
      } else if(c == '\n' || c == '\r' || !c) {
        /* <quote><cr/nl> ends row */
        status = sv_parse_save_cell(t);
        if(status)
          return status;

        sv_parse_generate_row(t);
        sv_parse_prepare_for_new_row(t);
        t->state = (!c ? SV_STATE_START_ROW : SV_STATE_EOL);
      } else {
        /* FIXME: could check that <quote> is followed by <sep> */
        status = sv_parse_cell_add_char(t, c);
        if(status)
          return status;

        t->state = SV_STATE_IN_CELL;
      }
      break;

    case SV_STATE_EOL:
      if(c == '\n' || c == '\r')
        ;
      else if(c) {
        t->state = SV_STATE_START_ROW;
        goto redo;
      } else {
        t->bad_records++;
        if(t->flags & SV_FLAGS_BAD_DATA_ERROR) {
#if defined(SV_DEBUG) && SV_DEBUG > 1
          fprintf(stderr, "Error in line %d: newline seen in unquoted cell\n",
                  t->line);
#endif
          return SV_STATUS_FAILED;
        }
      }
      break;

    case SV_STATE_COMMENT:
      if(c == '\n' || c == '\r')
        t->state = SV_STATE_EOL;
      break;

    case SV_STATE_UNKNOWN:
    default:
      break;
  }

  if(c && t->state != SV_STATE_EOL)
    sv_line_buffer_add_char(t, c);

  return SV_STATUS_OK;
}


/**
 * sv_internal_parse_chunk:
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
  sv_status_t status = SV_STATUS_OK;
  /* End of input if either of these is NULL */
  int is_end = (!buffer || !len);

  if(is_end) {
    if(sv_internal_parse_process_char(t, 0))
      status = SV_STATUS_FAILED;
  } else {
    while(len--) {
      char c = *buffer++;
      if(sv_internal_parse_process_char(t, c)) {
        status = SV_STATUS_FAILED;
        goto done;
      }
    }
  }

done:
  return status;
}
