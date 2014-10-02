/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * gen.c - SV generator example program
 *
 * Copyright (C) 2014, David Beckett http://www.dajobe.org/
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


/*
https://news.ycombinator.com/item?id=7795451

2 rows 5 columns

INPUT:

"x","""x""",,"x
x","x"
"y",,,,123


*/

#define N_FIELDS 5
const char* fields[N_FIELDS] = { "a", "b",     "c", "d",    "e"   };
const char* row_1[N_FIELDS]  = { "x", "\"x\"", "",  "x\nx", "x"   };
const char* row_2[N_FIELDS]  = { "y", "",      "",  "",     "123" };


const char* program;

int
main(int argc, char *argv[])
{
  int rc = 0;
  const char* data_file = NULL;
  FILE *fh = NULL;
  sv *t = NULL;
  size_t data_file_len;
  char sep = '\t'; /* default is TSV */

  program = "example";

  if(argc != 2) {
    fprintf(stderr, "USAGE: %s [SV FILE]\n", program);
    rc = 1;
    goto tidy;
  }

  data_file = (const char*)argv[1];
  fh = fopen(data_file, "w");
  if(!fh) {
    fprintf(stderr, "%s: Failed to write to data file %s: %s\n",
            program, data_file, strerror(errno));
    rc = 1;
    goto tidy;
  }

  data_file_len = strlen(data_file);

  if(data_file_len > 4) {
    if(!strcmp(data_file + data_file_len - 3, "csv"))
      sep = ',';
    else if(!strcmp(data_file + data_file_len - 3, "tsv"))
      sep = '\t';
  }

  fprintf(stderr, "%s: Using separator '%c'\n", program, sep);

  /* save first line as header not data */
  t = sv_new(NULL, NULL, NULL, sep);
  if(!t) {
    fprintf(stderr, "%s: Failed to init SV library", program);
    rc = 1;
    goto tidy;
  }

  sv_write_fields(t, fh, (char**)fields, NULL, N_FIELDS);
  sv_write_fields(t, fh, (char**)row_1, NULL, N_FIELDS);
  sv_write_fields(t, fh, (char**)row_2, NULL, N_FIELDS);

 tidy:
  if(t)
    sv_free(t);

  if(fh) {
    fclose(fh);
    fh = NULL;
  }

  return rc;
}
