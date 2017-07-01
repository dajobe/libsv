/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * write.c - Write SV
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

#include <sv.h>
#include "sv_internal.h"


/**
 * sv_write_field:
 * @t: sv object
 * @fh: file handle to write to
 * @field: field to write
 * @width: width of @field
 *
 * INTERNAL: Write a SV formatted field to a file handle
 */
static sv_status_t
sv_write_field(sv* t, FILE* fh, const char* field, size_t width)
{
  int needs_quote = 0;
  const char *p;

  for(p = field; *p ; p++) {
    if(*p == t->field_sep || *p == t->quote_char || *p == t->escape_char ||
       *p == '\r' || *p == '\n') {
      needs_quote = 1;
      break;
    }
  }

  if(needs_quote) {
    fputc(t->quote_char, fh);
    for(p = field; *p ; p++) {
      if(*p == t->field_sep || *p == t->quote_char) {
        /* Escape the field separator or quote char */
        if(*p == t->quote_char && (t->flags & SV_FLAGS_DOUBLE_QUOTE))
          fputc(*p, fh);
        else
          fputc(t->escape_char, fh);
      } else if(*p == t->escape_char) {
        /* Escape the escape char if defined */
        fputc(t->escape_char, fh);
      }
      fputc(*p, fh);
    }
    fputc(t->quote_char, fh);
  } else
    fwrite(field, 1, width, fh);

  return SV_STATUS_OK;
}



/**
 * sv_write_fields:
 * @t: sv object
 * @fh: FILE handle to write to
 * @fields: array of fields of length @count
 * @widths: array of widths of length @count (or NULL)
 * @count: number of fields
 *
 * Write a row of fields to a file handle with escaping
 */
sv_status_t
sv_write_fields(sv *t, FILE* fh, char** fields, size_t *widths, size_t count)
{
  sv_status_t status = SV_STATUS_OK;
  size_t i;

  for(i = 0; i < count; i++) {
    char* field = fields[i];
    size_t width;
    if(!field)
      break;

    if(i > 0)
      fputc(t->field_sep, fh);

    width = widths ? widths[i] : strlen(field);
    sv_write_field(t, fh, field, width);
  }
  fputc('\n', fh);

  return status;
}
