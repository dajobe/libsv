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


struct sv_s {
  /* field separator: '\t' or ',' */
  char field_sep;

  int line;

  /* row callback */
  void *callback_user_data;
  sv_fields_callback header_callback;
  sv_fields_callback data_callback;

  /* current buffer */
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
  size_t fields_buffer_size;

  /* first row is saved as headers */
  char **headers;
  size_t *headers_widths;

  unsigned int flags;

  /* error state */
  sv_status_t status;

  int bad_records;

  char last_char;

  char quote_char;

  /* called with the line (before parsing) */
  sv_line_callback line_callback;
};


sv_status_t sv_internal_parse_chunk(sv *t, char *buffer, size_t len);

#endif
