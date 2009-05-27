/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * tsv.c - Parse TSV files
 *
 * Copyright (C) 2009, David Beckett http://www.dajobe.org/
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
 * See LICENSE.html or LICENSE.txt at the top of this package for the
 * complete terms and further detail along with the license texts for
 * the licenses in COPYING.LIB, COPYING and LICENSE-2.0.txt respectively.
 * 
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <string.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif


#include <tsv.h>


struct tsv_s {
  int line;
  
  /* row callback */
  void *callback_user_data;
  tsv_fields_callback callback;

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
};


tsv*
tsv_init(void *user_data, tsv_fields_callback callback, int flags)
{
  tsv *t;
  
  t = (tsv*)malloc(sizeof(*t));
  if(!t)
    return NULL;
  
  t->line = 0;
  
  t->callback_user_data = user_data;
  t->callback = callback;

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
  
  return t;
}


static int
tsv_init_fields(tsv *t) 
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

  return 0;
  

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

  return 0;
}


void
tsv_free(tsv *t)
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
tsv_ensure_fields_buffer_size(tsv *t, size_t len)
{
  char *nbuffer;
  size_t nsize;
  
  if(len < t->fields_buffer_size)
    return 0;
  
  nsize = len + 8;

#if TSV_DEBUG
  fprintf(stderr, "%d: Growing buffer from %d to %d bytes\n",
          t->line, (int)t->fields_buffer_size, (int)nsize);
#endif
  
  nbuffer = (char*)malloc(nsize + 1);
  if(!nbuffer)
    return 1;

  if(t->fields_buffer)
    free(t->fields_buffer);
  
  t->fields_buffer = nbuffer;
  t->fields_buffer_size = nsize;

  return 0;
}



/* Ensure internal buffer is big enough for len more bytes */
static int
tsv_ensure_line_buffer_size(tsv *t, size_t len)
{
  char *nbuffer;
  size_t nsize;
  
  if(t->len + len < t->size)
    return 0;
  
  nsize = (len + t->len) << 1;
    
  nbuffer = (char*)malloc(nsize + 1);
  if(!nbuffer)
    return 1;

  if(t->len)
    memcpy(nbuffer, t->buffer, t->len);
  nbuffer[t->len] = '\0';
  
  if(t->buffer)
    free(t->buffer);
  
  t->buffer = nbuffer;
  t->size = nsize;

  return 0;
}


int
tsv_get_line(tsv *t)
{
  if(!t)
    return -1;

  return t->line;
}


const char*
tsv_get_header(tsv *t, unsigned int i, size_t *width_p)
{
  if(!t || !t->headers || i > t->fields_count)
    return NULL;

  if(width_p)
    *width_p = t->headers_widths[i];
  
  return (const char*)t->headers[i];
}



static int
tsv_parse_line(tsv *t, char *line, size_t len,  unsigned int* field_count_p)
{
  unsigned int column;
  int quote_count = 0;
  int field_width = 0;
  int field_offset = 0;
  char* current_field = NULL;
  char* p = NULL;
  char** fields = t->fields;
  size_t* fields_widths = t->fields_widths;

#if TSV_DEBUG
  if(fields) {
    fprintf(stderr, "Parsing line (%d bytes)\n  >>", (int)len);
    fwrite(line, 1, len, stderr);
    fputs("<<\n", stderr);
  }
#endif
  
  if(tsv_ensure_fields_buffer_size(t, len))
    return 1;

  if(fields) {
    current_field = t->fields_buffer;
    p = current_field;

    if(!p)
      return 1;
  }

  for(column = 0; 1; column++) {
    int c = -1;

    if(column == len)
      goto do_last;
    
    c= line[column];

    if(c == '"')
      quote_count++;
    else
      quote_count = 0;

#if TSV_DEBUG
      if(fields) {
        fprintf(stderr, "  Column %d: c %c  qc %d  width %d\n", column, c, quote_count, (int)field_width);
      }
#endif
      
    if(quote_count > 1)
      /* skip repeated quote - so it just replaces ""... with " */
      continue;


    do_last:
    if(c == '\t' || column == len) {
      if(p)
        *p++ = '\0';
      
      if(fields) {
        /* Remove quotes around quoted field */
        if(current_field[0] == '"' && 
           current_field[field_width-1] == '"') {
          field_width -= 2;

          /* save a memcpy: move the start of the field forward a byte */
          /* memcpy(&current_field[0], &current_field[1], field_width); */
          current_field++;

          current_field[field_width] = '\0';
        }
      }

#if TSV_DEBUG
      if(fields) {
        fprintf(stderr, "  Field %d: width %d\n", (int)field_offset, (int)field_width);
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

      field_offset++;
      current_field = p;

      continue;
    }
    
    if(fields)
      *p++ = c;
    field_width++;
  }


  if(field_count_p)
    *field_count_p = field_offset + 1;


  return 0;
}


int
tsv_parse_chunk(tsv *t, char *buffer, size_t len)
{
  unsigned int offset = 0;
  int rc = 0;
  
  if(tsv_ensure_line_buffer_size(t, len))
    return 1;
  
  /* add new buffer */
  memcpy(t->buffer + t->len, buffer, len);

  /* always ensure it is NUL terminated even if input chunk was not */
  t->len += len;
  t->buffer[t->len] = '\0';

  /* look for an end of line to do some work */
  for(offset = 0; offset < t->len; offset++) {
    size_t line_len;
    char *fields_buffer;
    unsigned int fields_count;

    if(t->buffer[offset] != '\n')
      continue;

    /* found a line */
    fields_count = 0;
    line_len = offset;

    if(!t->fields_count) {
      /* First line in the file - calculate number of fields */
      if(tsv_parse_line(t, t->buffer, line_len, &t->fields_count))
        return 1;

      /* initialise arrays of size t->fields_count */
      if(tsv_init_fields(t)) {
        free(fields_buffer);
        return 1;
      }
    }
    
    if(tsv_parse_line(t, t->buffer, line_len, &fields_count)) {
      free(fields_buffer);
      return 1;
    }

    if(fields_count != t->fields_count) {
      /** ERROR **/
      fprintf(stderr, "line %d: saw %d fields expected %d\n",
              t->line, fields_count, t->fields_count);
      return 1;
    }


    if(!t->line && (t->flags & TSV_FLAGS_SAVE_HEADER)) {
      /* first line so save fields as headers */
      unsigned int i;
      
      for(i = 0; i < t->fields_count; i++) {
        char *s = (char*)malloc(t->fields_widths[i]+1);
        memcpy(s, t->fields[i], t->fields_widths[i]+1);
        t->headers[i] = s;
      }
      goto skip;
    }


    /* got fields - return them to user */
    rc = t->callback(t, t->callback_user_data, t->fields, t->fields_widths,
                     t->fields_count);

    skip:
    
    /* adjust buffer - remove 'line_len+1' bytes from start of buffer */
    t->len -= line_len+1;

    /* this is an overlapping move */
    memmove(t->buffer, &t->buffer[line_len+1], t->len);

    /* This is not needed: guaranteed above */
    /* t->buffer[t->len] = '\0' */

    offset = 0;
    
    t->line++;
    
    if(rc)
      break;
  }
  
  return rc;
}
