/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * sv.c - Parse separated-values (CSV, TSV) files
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


#ifdef SV_CONFIG
#include <sv_config.h>
#endif

#include <stdio.h>
#include <string.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <sv.h>


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

  int flags;

  /* error state */
  sv_status_t status;

  int bad_records;
};


sv*
sv_init(void *user_data, sv_fields_callback header_callback,
        sv_fields_callback data_callback,
        char field_sep, int flags)
{
  sv *t;

  if(field_sep != '\t' && field_sep != ',')
    return NULL;
  
  t = (sv*)malloc(sizeof(*t));
  if(!t)
    return NULL;
  
  t->field_sep = field_sep;

  t->line = 1;
  
  t->callback_user_data = user_data;
  t->header_callback = header_callback;
  t->data_callback = data_callback;

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

  t->flags = flags;

  t->status = SV_STATUS_OK;

  t->bad_records = 0;

  return t;
}


static int
sv_init_fields(sv *t) 
{
  t->fields = (char**)malloc(sizeof(char*) * (t->fields_count+1));
  if(!t->fields)
    goto failed;
    
  t->fields_widths = (size_t*)malloc(sizeof(size_t*) * (t->fields_count+1));
  if(!t->fields_widths)
    goto failed;

  t->headers = (char**)malloc(sizeof(char*) * (t->fields_count+1));
  if(!t->headers)
    goto failed;
  
  t->headers_widths = (size_t*)malloc(sizeof(size_t*) * (t->fields_count+1));
  if(!t->headers_widths)
    goto failed;

  return SV_STATUS_OK;
  

  failed:
  if(t->fields) {
    free(t->fields);
    t->fields = NULL;
  }

  if(t->fields_widths) {
    free(t->fields_widths);
    t->fields_widths = NULL;
  }
  
  if(t->headers) {
    free(t->headers);
    t->headers = NULL;
  }

  return SV_STATUS_NO_MEMORY;
}


void
sv_free(sv *t)
{
  if(!t)
    return;

  if(t->headers_widths)
    free(t->headers_widths);
  if(t->headers) {
    unsigned int i;
    
    for(i = 0; i < t->fields_count; i++)
      free(t->headers[i]);
    free(t->headers);
  }
  

  if(t->fields_buffer)
    free(t->fields_buffer);

  if(t->fields_widths)
    free(t->fields_widths);
  if(t->fields)
    free(t->fields);
  if(t->buffer)
    free(t->buffer);
  
  free(t);
}



/* Ensure fields buffer is big enough for len bytes total */
static int
sv_ensure_fields_buffer_size(sv *t, size_t len)
{
  char *nbuffer;
  size_t nsize;
  
  if(len < t->fields_buffer_size)
    return SV_STATUS_OK;
  
  nsize = len + 8;

#if defined(SV_DEBUG) && SV_DEBUG > 1
  fprintf(stderr, "%d: Growing buffer from %d to %d bytes\n",
          t->line, (int)t->fields_buffer_size, (int)nsize);
#endif
  
  nbuffer = (char*)malloc(nsize + 1);
  if(!nbuffer)
    return SV_STATUS_NO_MEMORY;

  if(t->fields_buffer)
    free(t->fields_buffer);
  
  t->fields_buffer = nbuffer;
  t->fields_buffer_size = nsize;

  return SV_STATUS_OK;
}



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


int
sv_get_line(sv *t)
{
  if(!t)
    return -1;

  return t->line;
}


const char*
sv_get_header(sv *t, unsigned int i, size_t *width_p)
{
  if(!t || !t->headers || i > t->fields_count)
    return NULL;

  if(width_p)
    *width_p = t->headers_widths[i];
  
  return (const char*)t->headers[i];
}


#if defined(SV_DEBUG)
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
  int quote_count = 0;
  int field_width = 0;
  int field_offset = 0;
  char* current_field = NULL;
  char* p = NULL;
  char** fields = t->fields;
  size_t* fields_widths = t->fields_widths;
  sv_status_t status;
  int field_is_quoted = 0;

#if defined(SV_DEBUG)
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

    if(c == '"') {
      if(!field_width) {
        field_is_quoted = 1;
#if defined(SV_DEBUG) && SV_DEBUG > 1
        fprintf(stderr, "Field is quoted\n");
#endif
        continue;
      } else if(column == len-1 || line[column+1] == t->field_sep) {
#if defined(SV_DEBUG) && SV_DEBUG > 1
        fprintf(stderr, "Field ended on quote + sep\n");
#endif
        field_ended = 1;
        expect_sep = 1;
        goto do_last;
      } else {
#if defined(SV_DEBUG) && SV_DEBUG > 1
        fprintf(stderr, "Inner quote\n");
#endif
        /* inner quote */
        quote_count++;
      }
    } else
      quote_count = 0;

    if(quote_count == 2) {
#if defined(SV_DEBUG) && SV_DEBUG > 1
      fprintf(stderr, "Double quote absorbed\n");
#endif
      quote_count = 0;
      /* skip repeated quote - so it just replaces ""... with " */
      goto skip;
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
        /* Remove quotes around quoted field */
        if(field_width > 1 &&
           field_is_quoted &&
           current_field[0] == '"' && 
           current_field[field_width-1] == '"') {
          field_width -= 2;

          /* save a memcpy: move the start of the field forward a byte */
          /* memcpy(&current_field[0], &current_field[1], field_width); */
          current_field++;

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
      quote_count = 0;
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


sv_status_t
sv_parse_chunk(sv *t, char *buffer, size_t len)
{
  unsigned int offset = 0;
  sv_status_t status = SV_STATUS_OK;
  
  status = sv_ensure_line_buffer_size(t, len);
  if(status)
    return status;
  
  /* add new buffer */
  memcpy(t->buffer + t->len, buffer, len);

  /* always ensure it is NUL terminated even if input chunk was not */
  t->len += len;
  t->buffer[t->len] = '\0';

  /* look for an end of line to do some work */
  for(offset = 0; offset < t->len; offset++) {
    size_t line_len;
    unsigned int fields_count = 0;
    sv_status_t rc;
    
    if(t->buffer[offset] != '\n')
      continue;

#if defined(SV_DEBUG) && SV_DEBUG > 1
    sv_dump_buffer(stderr, "Starting buffer", t->buffer, t->len);
#endif

    /* found a line */
    fields_count = 0;
    line_len = offset;

    if(!line_len)
      goto skip_line;

    if(!t->fields_count) {
      /* First line in the file - calculate number of fields */
      rc = sv_parse_line(t, t->buffer, line_len, &t->fields_count);
      if(rc)
        return rc;

      /* initialise arrays of size t->fields_count */
      rc = sv_init_fields(t);
      if(rc) {
        if(t->fields_buffer) {
          free(t->fields_buffer);
          t->fields_buffer_size = 0;
        }
        return rc;
      }
    }
    
    rc = sv_parse_line(t, t->buffer, line_len, &fields_count);
    if(rc) {
      if(t->fields_buffer) {
        free(t->fields_buffer);
        t->fields_buffer_size = 0;
      }
      
      return rc;
    }

    if(fields_count != t->fields_count) {
      t->bad_records++;
      if(t->flags & SV_FLAGS_BAD_DATA_ERROR) {
#if defined(SV_DEBUG)
        fprintf(stderr, "Error in line %d: saw %d fields expected %d\n",
                t->line, fields_count, t->fields_count);
#endif
        return SV_STATUS_LINE_FIELDS;
      }
#if defined(SV_DEBUG)
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
        memcpy(s, t->fields[i], t->fields_widths[i]+1);
        t->headers[i] = s;
        t->headers_widths[i] = t->fields_widths[i];
      }

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
        

    skip_line:
    
    /* adjust buffer - remove 'line_len+1' bytes from start of buffer */
    t->len -= line_len+1;

    /* this is an overlapping move */
    memmove(t->buffer, &t->buffer[line_len+1], t->len);

    /* This is not needed: guaranteed above */
    /* t->buffer[t->len] = '\0' */

    t->line++;
    offset = -1; /* so for loop starts at 0 */
    
    if(status)
      break;
  }
  
  return status;
}
