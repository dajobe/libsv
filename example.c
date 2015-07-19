/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * example.c - SV example program
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
#include <ctype.h>

#include <sv.h>


/* structure used for user data callback */
typedef struct 
{
  const char* filename;
  int count;
  char* line;
} myc;


const char* program;


static void
my_sv_dump_string(FILE* fh, const char* buffer, size_t len)
{
  size_t mylen = len;
  size_t i;

  if(!buffer) {
    fwrite("NULL", sizeof(char), 4, fh);
    return;
  }

  if(mylen > 100)
    mylen = 100;
  for(i = 0; i < mylen; i++) {
    const char c = buffer[i];
    if(isprint(c))
      fputc(c, fh);
    else
      fprintf(fh, "\\x%02X", c);
  }
  if(mylen != len)
    fputs("...", fh);
}


static void
my_sv_dump_buffer(FILE* fh, const char* buffer, size_t len)
{
  fwrite(">>>", sizeof(char), 3, fh);
  my_sv_dump_string(fh, buffer, len);
  fprintf(fh, "<<< (%zu bytes)\n", len);
}


static sv_status_t
my_sv_line_callback(sv *t, void *user_data, const char* line, size_t length)
{
  myc *c = (myc*)user_data;

  if(c->line)
    free(c->line);
  c->line = (char*)malloc(length + 1);
  if(c->line)
    memcpy(c->line, line, length + 1);

  fprintf(stdout, "%s:%d: Line ", c->filename, sv_get_line(t));
  my_sv_dump_buffer(stdout, line, length);

  /* This code always succeeds */
  return SV_STATUS_OK;
}



static sv_status_t
my_sv_header_callback(sv *t, void *user_data,
                      char** headers, size_t *widths, size_t count)
{
  unsigned int i;
  myc *c = (myc*)user_data;

  fprintf(stdout, "%s:%d: Header with %d fields\n",
          c->filename, sv_get_line(t), (int)count);
  for(i = 0; i < count; i++) {
    fprintf(stdout, "%3d: '", (int)i);
    my_sv_dump_string(stdout, headers[i], widths[i]);
    fprintf(stdout, "' (width %d)\n", (int)widths[i]);
  }

  /* This code always succeeds */
  return SV_STATUS_OK;
}


static sv_status_t
my_sv_fields_callback(sv *t, void *user_data,
                       char** fields, size_t *widths, size_t count)
{
  unsigned int i;
  myc *c = (myc*)user_data;

  c->count++;

  fprintf(stdout, "%s:%d: Record with %d fields\n",
          c->filename, sv_get_line(t), (int)count);
  for(i = 0; i < count; i++) {
    const char* header = sv_get_header(t, i, NULL);
    fprintf(stdout, "%3d %-10s: '", (int)i, header ? header : "NULL");
    my_sv_dump_string(stdout, fields[i], widths[i]);
    fprintf(stdout, "' (width %d)\n", (int)widths[i]);
  }
  /* This code always succeeds */
  return SV_STATUS_OK;
}


static sv_status_t
my_sv_comment_callback(sv *t, void *user_data,
                       const char* comment, size_t length)
{
  myc *c = (myc*)user_data;
  fprintf(stdout, "%s:%d: Comment ", c->filename, sv_get_line(t));
  my_sv_dump_buffer(stdout, comment, length);

  /* This code always succeeds */
  return SV_STATUS_OK;
}



int
main(int argc, char *argv[])
{
  int rc = 0;
  const char* data_file = NULL;
  FILE *fh = NULL;
  sv *t = NULL;
  myc c;
  size_t data_file_len;
  char sep = '\t'; /* default is TSV */
  int option_save_header = 1; /* default is to expect a header line */
  int usage =0;

  program = "example";

  memset(&c, '\0', sizeof(c));

  if(argc < 1 || argc > 3) {
    usage = 1;
  } else if(argc == 3) {
    if(!strcmp(argv[1], "-n"))
      option_save_header = 0;
    else {
      fprintf(stderr, "%s: Unknown option %s\n", program, argv[1]);
      usage = 1;
    }
    argv++;
    argc--;
  }

  if(usage) {
    fprintf(stderr, "USAGE: %s [OPTIONS] [SV FILE]\n", program);
    fputs("-n: No header present\n", stderr);
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

  c.filename = data_file;
  c.count = 0;
  c.line = NULL;

  data_file_len = strlen(data_file);

  if(data_file_len > 4) {
    if(!strcmp(data_file + data_file_len - 3, "csv"))
      sep = ',';
    else if(!strcmp(data_file + data_file_len - 3, "tsv"))
      sep = '\t';
  }

  fprintf(stderr, "%s: Using separator '%c'\n", program, sep);

  /* save first line as header not data */
  t = sv_new(&c, my_sv_header_callback, my_sv_fields_callback, sep);
  if(!t) {
    fprintf(stderr, "%s: Failed to init SV library", program);
    rc = 1;
    goto tidy;
  }

  sv_set_option(t, SV_OPTION_SAVE_HEADER, option_save_header);
  sv_set_option(t, SV_OPTION_LINE_CALLBACK, my_sv_line_callback);
  sv_set_option(t, SV_OPTION_COMMENT_CALLBACK, my_sv_comment_callback);

  while(!feof(fh)) {
    char buffer[1024];
    size_t len = fread(buffer, 1, sizeof(buffer), fh);

    if(sv_parse_chunk(t, buffer, len))
      break;
  }
  fclose(fh);
  fh = NULL;

  /* Record EOF */
  sv_parse_chunk(t, NULL, 0);

  fprintf(stderr, "%s: Saw %d records\n", program, c.count);

 tidy:
  if(c.line)
    free(c.line);

  if(t)
    sv_free(t);

  if(fh) {
    fclose(fh);
    fh = NULL;
  }

  return rc;
}
