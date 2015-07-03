/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * parse2.c - SV parsing V2
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
  int cell_ix = t->fields_count;
  int cell_len = t->fields_buffer_len;

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
    memcpy(s, t->fields_buffer, cell_len + 1);
  s[cell_len] = '\0';

  t->fields[cell_ix] = s;
  t->fields_widths[cell_ix] = cell_len;

#if defined(SV_DEBUG) && SV_DEBUG > 2
  fprintf(stderr, "Line %d Cell %d >>", t->line, cell_ix);
  fwrite(s, 1, cell_len, stderr);
  fprintf(stderr, "<< (%d)\n", cell_len);
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
 * INTERNAL - process one character; NUL indicates end of input
 *
 * Return value: non-0 on failure
 */

static sv_status_t
sv_parse_generate_row(sv *t)
{
  sv_status_t status = SV_STATUS_OK;

#if defined(SV_DEBUG) && SV_DEBUG > 2
  fprintf(stderr, "Generating row %d\n", t->line);
#endif

  if(t->line == 1 && (t->flags & SV_FLAGS_SAVE_HEADER)) {
    int nfields = t->fields_count;
    char** cp;
    size_t* sp;

    /* first line and header: turn fields into headers */
    cp = (char**)malloc(sizeof(char*) * (nfields + 1));
    if(!cp)
      return SV_STATUS_NO_MEMORY;
    t->headers = cp;

    sp = (size_t*)malloc(sizeof(size_t) * (nfields + 1));
    if(!sp)
      return SV_STATUS_NO_MEMORY;
    t->headers_widths = sp;

    if(nfields > 0) {
      memcpy(cp, t->fields, sizeof(char*) * nfields + 1);
      memcpy(sp, t->fields_widths, sizeof(size_t) * nfields + 1);
    }
  }

  if(t->line_callback) {
    status = t->line_callback(t, t->callback_user_data, "fake line", 9);
    if(status != SV_STATUS_OK)
      return status;
  }

  if(t->line == 1 && (t->flags & SV_FLAGS_SAVE_HEADER)) {
    if(t->header_callback) {
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

  t->fields_count = 0;

  t->line++;

  return status;
}

#if defined(SV_DEBUG) && SV_DEBUG > 2
static const char* const sv_state_labels[SV_STATE_LAST + 1] = {
  "UNKNOWN",
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
    case SV_STATE_START_FILE:
      t->state = SV_STATE_START_ROW;
      /* FIXME - add BOM checking for encoding */
      /* FALLTHROUGH */

    gsr:
    case SV_STATE_START_ROW:
      if(!c)
        break;
      else if(c == '\n' || c == '\r') {
        t->state = SV_STATE_EOL;
        break;
      } else if(c == '#') {
        t->state = SV_STATE_COMMENT;
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
        t->state = (!c ? SV_STATE_START_ROW : SV_STATE_EOL);
      } else if(c == t->quote_char) {
        t->state = SV_STATE_IN_QUOTED_CELL;
      } else if(c == t->escape_char) {
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
        t->state = (!c ? SV_STATE_START_ROW : SV_STATE_EOL);
      } else if(c == t->escape_char) {
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
      else if(c == t->escape_char) {
        t->state = SV_STATE_ESC_IN_QUOTED_CELL;
      } else if(c == t->quote_char) {
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
      if(c == t->quote_char) {
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
