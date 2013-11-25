/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example.c - SV example program
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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#include <sv.h>


/* structure used for user data callback */
typedef struct 
{
  int count;
} myc;


static sv_status_t
my_sv_header_callback(sv *t, void *user_data,
                      char** fields, size_t *widths, size_t count)
{
  unsigned int i;
  
  fprintf(stdout, "Header with %d fields\n",  (int)count);
  for(i = 0; i < count; i++)
    fprintf(stdout, "%3d: '%s' (width %d)\n", (int)i,
            fields[i], (int)widths[i]);

  /* This code always succeeds */
  return SV_STATUS_OK;
}


static sv_status_t
my_sv_fields_callback(sv *t, void *user_data,
                       char** fields, size_t *widths, size_t count)
{
  unsigned int i;
  myc *c=(myc*)user_data;
  
  c->count++;
  
  fprintf(stdout, "Line %d: Record with %d fields\n",
          sv_get_line(t), (int)count);
  for(i = 0; i < count; i++)
    fprintf(stdout, 
            "%3d %-10s: '%s' (width %d)\n", (int)i,
            sv_get_header(t, i, NULL), fields[i], (int)widths[i]);

  /* This code always succeeds */
  return SV_STATUS_OK;
}


const char *program="example";

int
main(int argc, char *argv[])
{
  int rc = 0;
  const char* data_file = NULL;
  FILE *fh = NULL;
  sv *t = NULL;
  myc c;
  
  if(argc != 2) {
    fprintf(stderr, "USAGE: %s [SV FILE]\n", program);
    rc = 1;
    goto tidy;
  }

  data_file = (const char*)argv[1];
  if(access(data_file, R_OK)) {
    fprintf(stderr, "%s: Failed to find data file %s\n",
            program, data_file);
    rc = 1;
    goto tidy;
  }

  fh = fopen(data_file, "r");
  if(!fh) {
    fprintf(stderr, "%s: Failed to read data file %s: %s\n",
            program, data_file, strerror(errno));
    rc = 1;
    goto tidy;
  }

  c.count = 0;

  /* save first line as header not data */
  t = sv_init(&c, my_sv_header_callback, my_sv_fields_callback, 
              '\t', SV_FLAGS_SAVE_HEADER);
  if(!t) {
    fprintf(stderr, "%s: Failed to init SV library", program);
    rc = 1;
    goto tidy;
  }
  
  while(!feof(fh)) {
    char buffer[1024];
    size_t len = fread(buffer, 1, sizeof(buffer), fh);
    
    if(sv_parse_chunk(t, buffer, len))
      break;
  }
  fclose(fh);
  fh = NULL;
  
  fprintf(stderr, "%s: Saw %d records\n", program, c.count);
  
 tidy:
  if(t)
    sv_free(t);

  if(fh) {
    fclose(fh);
    fh = NULL;
  }
  
  return rc;
}
