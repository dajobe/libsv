/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * sv_internal.h - Internal definitions for libsv
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

#ifndef SV_INTERNAL_H
#define SV_INTERNAL_H 1

/* bit flags */
#define SV_FLAGS_SAVE_HEADER    (1<<0)
/* error out on bad data lines */
#define SV_FLAGS_BAD_DATA_ERROR (1<<1)
/* allow fields to be quoted */
#define SV_FLAGS_QUOTED_FIELDS  (1<<2)
/* strip (non-separator) whitespace around fields */
#define SV_FLAGS_STRIP_WHITESPACE  (1<<3)
/* double a quote to quote it (primarily for ") */
#define SV_FLAGS_DOUBLE_QUOTE      (1<<4)

typedef enum  {
  SV_STATE_UNKNOWN,
  /* After a reset and before any potential BOM */
  SV_STATE_START_FILE,
  /* After any BOM and expecting record start */
  SV_STATE_START_ROW,
  /* Accepted \r or \n - new line; handle \r\n as 1 EOL */
  SV_STATE_EOL,
  /* Escaped newline */
  SV_STATE_ESC_EOL,
  /* Accepted # - read a commented row up to EOL */
  SV_STATE_COMMENT,
  /* Starting a cell */
  SV_STATE_START_CELL,
  /* In a cell */
  SV_STATE_IN_CELL,
  /* Accepted escape-char in a cell */
  SV_STATE_ESC_IN_CELL,
  /* Accepted quote-char starting a cell */
  SV_STATE_IN_QUOTED_CELL,
  /* Accepted escape-char in a quoted cell */
  SV_STATE_ESC_IN_QUOTED_CELL,
  /* Accepted quote in a quoted cell */
  SV_STATE_QUOTE_IN_QUOTED_CELL,
  SV_STATE_LAST = SV_STATE_QUOTE_IN_QUOTED_CELL
} sv_parse_state;


struct sv_s {
  /* field separator: '\t' or ',' */
  char field_sep;

  int line;

  /* row callback */
  void *callback_user_data;
  sv_fields_callback header_callback;
  sv_fields_callback data_callback;

  /* line buffer */
  char *buffer;
  /* size allocated */
  size_t size;
  /* size used */
  size_t len;

  unsigned int fields_count;
  char **fields;
  size_t *fields_widths;

  /* memory buffer used for constructing fields for user;
   * array above 'fields' points into this
   */
  char* fields_buffer;
  /* allocate size */
  size_t fields_buffer_size;
  /* used size */
  size_t fields_buffer_len;

  /* first row is saved as headers */
  unsigned int headers_count;
  char **headers;
  size_t *headers_widths;

  unsigned int flags;

  /* error state */
  sv_status_t status;

  int bad_records;

  char quote_char;

  /* called with the line (before parsing) */
  sv_line_callback line_callback;

  char escape_char;

  sv_parse_state state;
};

sv_status_t sv_internal_parse_chunk(sv *t, char *buffer, size_t len);

/* sv.c */
void sv_set_quote_char(sv *t, char quote_char);

/* common-parse.c */
sv_status_t sv_init_fields(sv *t, int nfields);
void sv_free_fields(sv *t);
void sv_reset_line_buffer(sv *t);
void sv_free_line_buffer(sv *t);
void sv_internal_parse_reset(sv* t);
sv_status_t sv_ensure_fields_buffer_size(sv *t, size_t len);
sv_status_t sv_ensure_line_buffer_size(sv *t, size_t len);
sv_status_t sv_line_buffer_add_char(sv* t, char c);

/* sv.c */
void sv_set_quote_char(sv *t, char quote_char);

#endif
