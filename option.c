/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * option.c - Options
 *
 * Copyright (C) 2009-2025, Dave Beckett http://www.dajobe.org/
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

#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <sv.h>
#include "sv_internal.h"


static sv_status_t
sv_set_option_vararg(sv* t, sv_option_t option, va_list arg)
{
  sv_status_t status = SV_STATUS_OK;

  switch(option) {
    case SV_OPTION_SAVE_HEADER:
      t->flags &= ~SV_FLAGS_SAVE_HEADER;
      if(va_arg(arg, long))
        t->flags |= SV_FLAGS_SAVE_HEADER;
      break;

    case SV_OPTION_BAD_DATA_ERROR:
      t->flags &= ~SV_FLAGS_BAD_DATA_ERROR;
      if(va_arg(arg, long))
        t->flags |= SV_FLAGS_BAD_DATA_ERROR;
      break;

    case SV_OPTION_QUOTED_FIELDS:
      t->flags &= ~SV_FLAGS_QUOTED_FIELDS;
      if(va_arg(arg, long))
        t->flags |= SV_FLAGS_QUOTED_FIELDS;
      break;

    case SV_OPTION_STRIP_WHITESPACE:
      t->flags &= ~SV_FLAGS_STRIP_WHITESPACE;
      if(va_arg(arg, long))
        t->flags |= SV_FLAGS_STRIP_WHITESPACE;
      break;

    case SV_OPTION_QUOTE_CHAR:
      if(1) {
        int c = va_arg(arg, int);
        if(c != t->field_sep)
          sv_internal_set_quote_char(t, c);
      }
      break;

    case SV_OPTION_LINE_CALLBACK:
      if(1) {
        sv_line_callback cb = (sv_line_callback)va_arg(arg, void*);
        t->line_callback = cb;
      }
      break;

    case SV_OPTION_DOUBLE_QUOTE:
      t->flags &= ~SV_FLAGS_DOUBLE_QUOTE;
      if(va_arg(arg, long))
        t->flags |= SV_FLAGS_DOUBLE_QUOTE;
      break;

    case SV_OPTION_ESCAPE_CHAR:
      if(1) {
        int c = va_arg(arg, int);
        t->escape_char = c;
      }
      break;

    case SV_OPTION_COMMENT_PREFIX:
      if(1) {
        char* s = va_arg(arg, char*);
        if(s && *s) {
          size_t l = strlen(s);
          char *new_s = (char*)malloc(l + 1);
          if(new_s) {
            memcpy(new_s, s, l + 1);
            if(t->comment_prefix)
              free(t->comment_prefix);
            t->comment_prefix = new_s;
            t->comment_prefix_len = l;
          } else
            status = SV_STATUS_FAILED;
        }
      }
      break;

    case SV_OPTION_SKIP_ROWS:
      if(1) {
        int c = va_arg(arg, int);
        t->skip_rows = c;
      }
      break;

    case SV_OPTION_COMMENT_CALLBACK:
      if(1) {
        sv_line_callback cb = (sv_line_callback)va_arg(arg, void*);
        t->comment_callback = cb;
      }
      break;

    case SV_OPTION_NULL_VALUES:
      if(1) {
        char** null_vals = (char**)va_arg(arg, char**);
        int count = va_arg(arg, int);
        
        /* Free existing null values if any */
        if(t->null_values) {
          unsigned int i;
          for(i = 0; i < t->null_values_count; i++) {
            if(t->null_values[i])
              free(t->null_values[i]);
          }
          free(t->null_values);
          free(t->null_values_lengths);
          t->null_values = NULL;
          t->null_values_count = 0;
          t->null_values_lengths = NULL;
        }
        
        if(null_vals && count > 0) {
          unsigned int i;
          t->null_values = (char**)malloc(sizeof(char*) * count);
          t->null_values_lengths = (size_t*)malloc(sizeof(size_t) * count);
          
          if(!t->null_values || !t->null_values_lengths) {
            if(t->null_values)
              free(t->null_values);
            if(t->null_values_lengths)
              free(t->null_values_lengths);
            t->null_values = NULL;
            t->null_values_lengths = NULL;
            status = SV_STATUS_NO_MEMORY;
            break;
          }
          
          for(i = 0; i < count; i++) {
            if(null_vals[i]) {
              size_t len = strlen(null_vals[i]);
              t->null_values[i] = (char*)malloc(len + 1);
              if(!t->null_values[i]) {
                /* Cleanup on failure */
                unsigned int j;
                for(j = 0; j < i; j++) {
                  if(t->null_values[j])
                    free(t->null_values[j]);
                }
                free(t->null_values);
                free(t->null_values_lengths);
                t->null_values = NULL;
                t->null_values_lengths = NULL;
                status = SV_STATUS_NO_MEMORY;
                break;
              }
              memcpy(t->null_values[i], null_vals[i], len + 1);
              t->null_values_lengths[i] = len;
            } else {
              t->null_values[i] = NULL;
              t->null_values_lengths[i] = 0;
            }
          }
          t->null_values_count = count;
        }
      }
      break;

    case SV_OPTION_NULL_HANDLING:
      t->flags &= ~SV_FLAGS_NULL_HANDLING;
      if(va_arg(arg, long))
        t->flags |= SV_FLAGS_NULL_HANDLING;
      break;

    default:
    case SV_OPTION_NONE:
      status = SV_STATUS_FAILED;
      break;

  }

  return status;
}


/**
 * sv_set_option:
 * @t: sv object
 * @option: option name
 *
 * Set an option value.  The value varies in type dependent on the @option
 *
 * Return value: #SV_STATUS_FAILED if failed
 */
sv_status_t
sv_set_option(sv *t, sv_option_t option, ...)
{
  sv_status_t status;
  va_list arg;

  va_start(arg, option);
  status = sv_set_option_vararg(t, option, arg);
  va_end(arg);

  return status;
}
