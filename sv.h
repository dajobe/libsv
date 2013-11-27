/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * sv.h - Header for libsv
 *
 * Copyright (C) 2009-2013, David Beckett http://www.dajobe.org/
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
  SV_STATUS_OK = 0,
  SV_STATUS_FAILED,
  SV_STATUS_NO_MEMORY,
  SV_STATUS_LINE_FIELDS
} sv_status_t;

typedef struct sv_s sv;
typedef sv_status_t (*sv_fields_callback)(sv *t, void *user_data, char** fields, size_t *widths, size_t count);

typedef enum {
  SV_OPTION_NONE = 0,
  SV_OPTION_SAVE_HEADER,
  SV_OPTION_BAD_DATA_ERROR
} sv_option_t;

sv* sv_init(void *user_data, sv_fields_callback header_callback, sv_fields_callback data_callback, char field_sep);
void sv_free(sv *t);

sv_status_t sv_set_option(sv *t, sv_option_t option, ...);

int sv_get_line(sv *t);
const char* sv_get_header(sv *t, unsigned int i, size_t *width_p);

sv_status_t sv_parse_chunk(sv *t, char *buffer, size_t len);

