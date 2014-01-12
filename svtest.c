/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * svtest.c - SV tests
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


char* program;


typedef struct 
{
  char sep;
  unsigned int flags;
  const char* data;
  const char** const expected;
  int columns_count;
  int rows_count;
} my_test_data;


/* structure used for user data callback */
typedef struct 
{
  int test_index;
  int columns_count;
  int rows_count;
  const my_test_data *expected;
  int header_errors;
  int data_errors;
} myc;


#define N_TESTS 2
static const char* const expected_1[4] = {"a", "b", "1", "2" };

static const my_test_data test_data[N_TESTS + 1] = {
  { ',',  0, "a,b\n1,2\n",   expected_1, 2, 1 },
  { '\t', 0, "a\tb\n1\t2\n", expected_1, 2, 1 },
  { '\0', 0, NULL,           NULL,       0, 0 }
};


#define EXPECTED_HEADER_IX(col) (col)
#define EXPECTED_DATA_IX(row, col) (((1+row) * c->expected->columns_count) + col)


static sv_status_t
my_sv_header_callback(sv *t, void *user_data,
                      char** fields, size_t *widths, size_t count)
{
  unsigned int column_i;
  myc *c = (myc*)user_data;

  c->columns_count = count;

  for(column_i = 0; column_i < count; column_i++) {
    const char* header = fields[column_i];
    unsigned int ix = EXPECTED_HEADER_IX(column_i);
    const char* expected_header = c->expected->expected[ix];

    if(strcmp(header, expected_header)) {
      fprintf(stderr, "%s: TEST %d failed - got header '%s' expected '%s'\n", 
              program, c->test_index, header, expected_header);
      c->header_errors++;
    }
  }

  return SV_STATUS_OK;
}


static sv_status_t
my_sv_fields_callback(sv *t, void *user_data,
                      char** fields, size_t *widths, size_t count)
{
  unsigned int column_i;
  myc *c = (myc*)user_data;

  for(column_i = 0; column_i < count; column_i++) {
    const char* data = fields[column_i];
    unsigned int ix = EXPECTED_DATA_IX(c->rows_count, column_i);
    const char* expected_data = c->expected->expected[ix];

    if(strcmp(data, expected_data)) {
      fprintf(stderr, "%s: TEST %d failed - got data '%s' expected '%s'\n", 
              program, c->test_index, data, expected_data);
      c->data_errors++;
    }
  }

  c->rows_count++;
  
  /* This code always succeeds */
  return SV_STATUS_OK;
}


int
main(int argc, char *argv[])
{
  char *p;
  int rc = 0;
  int errors = 0;
  unsigned int test_index;

  program = argv[0];
  if((p = strrchr(program, '/')))
    program = p + 1;
  else if((p = strrchr(program, '\\')))
    program = p + 1;
  argv[0] = program;
  
  if(argc != 1) {
    fprintf(stderr, "USAGE: %s\n", program);
    rc = 1;
    goto tidy;
  }


  for(test_index = 0; test_index < N_TESTS; test_index++) {
    myc c;
    size_t data_len;
    sv *t = NULL;
    const my_test_data *test = &test_data[test_index];
    sv_status_t status;

    memset(&c, '\0', sizeof(c));
    c.test_index = test_index;
    c.columns_count = 0;
    c.rows_count = 0;
    c.expected = test;

    data_len = strlen(test->data);

    t = sv_init(&c, my_sv_header_callback, my_sv_fields_callback, test->sep);
    if(!t) {
      fprintf(stderr, "%s: Failed to init SV library", program);
      rc = 1;
      goto tidy;
    }

    status = sv_parse_chunk(t, (char*)test->data, data_len);
    if(status != SV_STATUS_OK) {
      fprintf(stderr, "%s: Test %d FAIL - sv_parse_chunk() returned %d\n",
              program, test_index, (int)status);
      errors++;
    } else if(c.header_errors) {
      fprintf(stderr, "%s: Test %d FAIL - header errors\n",
              program, test_index);
      errors++;
    } else if(c.data_errors) {
      fprintf(stderr, "%s: Test %d FAIL - data errors\n",
              program, test_index);
      errors++;
    } else if(test->rows_count != c.rows_count) {
      fprintf(stderr, "%s: Test %d FAIL - saw %d records - expected %d\n",
              program, test_index, c.rows_count, test->rows_count); 
     errors++;
    } else {
      fprintf(stderr, "%s: Test %d OK\n",
              program, test_index);
    }

    sv_free(t);
  }

  rc = errors;

 tidy:

  return rc;
}