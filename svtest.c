/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * svtest.c - SV tests
 *
 * Copyright (C) 2014-2015, Dave Beckett http://www.dajobe.org/
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
  unsigned int option;
  const char* data;
  const char** const expected;
  int columns_count;
  int rows_count;
} svtest_data_set;


/* structure used for user data callback */
typedef struct 
{
  int test_index;
  /* expected results */
  const svtest_data_set *expected;

  /* raw line */
  char* line;
  size_t line_len;
  /* processed counts */
  int columns_count;
  int rows_count;
  /* errors seen */
  int header_errors;
  int data_errors;
} svtest_context;


#define N_TESTS 38
static const char* const expected_0[4] = {"a", "b", "1", "2" };
static const char* const expected_1[4] = {"c", "d", "3", "4" };
static const char* const expected_2[4] = {"e", "f", "5", "6" };

/* test.csv */
static const char* const expected_3[6] = {"a", "b", "c", "cat", "sat", "mat" };
static const char* const expected_4[6] = {"a", "b", "c", "", "sat", "mat" };
static const char* const expected_5[6] = {"a", "b", "c", "this is", "a", "test" };
static const char* const expected_6[6] = {"a", "b", "c", "this", "is a", "test" };
static const char* const expected_7[6] = {"a", "b", "c", "this", "is", "a test" };
static const char* const expected_8[6] = {"a", "b", "c", "this,is", "a", "test" };
static const char* const expected_9[6] = {"a", "b", "c", "this", "is,a", "test" };
static const char* const expected_10[6] = {"a", "b", "c", "this", "is", "a,test" };
static const char* const expected_11[6] = {"a", "b", "c", "\"", "\"", "\"" };
static const char* const expected_12[6] = {"a", "b", "c", "\"\"", "\"\"", "\"\"" };
static const char* const expected_13[6] = {"a", "b", "c", "\"\"\"", "\"\"\"", "\"\"\"" };
static const char* const expected_14[6] = {"a", "b", "c", "quoting", "can \"be\"", "fun" };
/* https://news.ycombinator.com/item?id=7795451 */
static const char* const expected_15[15] = {"a", "b", "c", "d", "e",
   "x","\"x\"","","x\nx","x",
   "y","","","","123" };
/* inspired by https://github.com/knrz/CSV.js/blob/master/test.js */
static const char* const expected_16[8] = {"a", "b", "c", "d", "1", "2", "3,4"};
static const char* const expected_17[8] = {"a", "b", "c", "d", "1", "2", "\"3,4\""};
static const char* const expected_18[8] = {"a", "b", "c", "d", "1", "2", "3\n4"};
/* inspired by https://github.com/d3/d3-dsv/blob/master/test/csv-test.js */
static const char* const expected_19[6] = {"a", "b", "c",  "1",  "2", "3" };
static const char* const expected_20[6] = {"a", "b", "c", " 1", " 2", "3" };
static const char* const expected_21[6] = {"a", "b", "c",  "1",  "2", "3" };
static const char* const expected_22[6] = {"a", "b", "c",  "1",  "2", "3" };
static const char* const expected_23[2] = {"a", "\"hello\"" };
static const char* const expected_24[2] = {"a", "new\nline" };
static const char* const expected_25[2] = {"a", "new\rline" };
static const char* const expected_26[2] = {"a", "new\r\nline" };
static const char* const expected_27[12] = {"a", "b", "c",  "1",  "2", "3", "4", "5", "6", "7", "8", "9" };
static const char* const expected_28[12] = {"a", "b", "c",  "1",  "2", "3", "4", "5", "6", "7", "8", "9" };
static const char* const expected_29[12] = {"a", "b", "c",  "1",  "2", "3", "4", "5", "6", "7", "8", "9" };
static const char* const expected_30[8] = {"a", "b", "c", "d", "col 1.1", "col 1.2\\", "col 1.3\\", "col\n1.4" };
static const char* const expected_31[6] = {"a", "b", "c", "cat", "sat", "mat" };


static const svtest_data_set svtest_data[N_TESTS + 1] = {
  { ',',  0, "a,b\n",        (const char** const)expected_0, 2, 0 },
  { '\t', 0, "a\tb",         (const char** const)expected_0, 2, 0 },
  { ',',  0, "a,b\n1,2\n",   (const char** const)expected_0, 2, 1 },
  { '\t', 0, "a\tb\n1\t2",   (const char** const)expected_0, 2, 1 },
  { '\t', 0, "c\td\n3\t4\n", (const char** const)expected_1, 2, 1 },
  { ',', SV_OPTION_STRIP_WHITESPACE, "  e\t , \tf  \n  5\t , \t 6\n", (const char** const)expected_2, 2, 1 },
  { '\t', 0, "c\td\n\n\n3\t4\n\n\n", (const char** const)expected_1, 2, 1 },
  { ',',  0, "c,d\n3,4",         (const char** const)expected_1, 2, 1 },

  /* test.csv */
  { ',',  0, "a,b,c\ncat,sat,mat\n", (const char** const)expected_3, 3, 1 },
  { ',',  0, "a,b,c\n,sat,mat\n", (const char** const)expected_4, 3, 1 },
  { ',',  0, "a,b,c\n\"this is\",a,test\n", (const char** const)expected_5, 3, 1 },
  { ',',  0, "a,b,c\nthis,\"is a\",test\n", (const char** const)expected_6, 3, 1 },
  { ',',  0, "a,b,c\nthis,is,\"a test\"\n", (const char** const)expected_7, 3, 1 },
  { ',',  0, "a,b,c\n\"this,is\",a,test\n", (const char** const)expected_8, 3, 1 },
  { ',',  0, "a,b,c\nthis,\"is,a\",test\n", (const char** const)expected_9, 3, 1 },
  { ',',  0, "a,b,c\nthis,is,\"a,test\"\n", (const char** const)expected_10, 3, 1 },
  { ',',  0, "a,b,c\n\"\"\"\",\"\"\"\",\"\"\"\"\n", (const char** const)expected_11, 3, 1 },
  { ',',  0, "a,b,c\n\"\"\"\"\"\",\"\"\"\"\"\",\"\"\"\"\"\"\n", (const char** const)expected_12, 3, 1 },
  { ',',  0, "a,b,c\n\"\"\"\"\"\"\"\",\"\"\"\"\"\"\"\",\"\"\"\"\"\"\"\"\n", (const char** const)expected_13, 3, 1 },
  { ',',  0, "a,b,c\nquoting,\"can \"\"be\"\"\",fun\n\"\n", (const char** const)expected_14, 3, 1 },
  /* https://news.ycombinator.com/item?id=7795451 */
  { ',', 0, "a,b,c,d,e\n\"x\",\"\"\"x\"\"\",,\"x\nx\",\"x\"\n\"y\",,,,123\n", (const char** const)expected_15, 5, 2 },
  /* inspired by https://github.com/knrz/CSV.js/blob/master/test.js */
  { ',', 0, "a,b,c,d\n1,2,\"3,4\"", (const char** const)expected_16, 4, 1 },
  { ',', 0, "a,b,c,d\n1,2,\"\"\"3,4\"\"\"", (const char** const)expected_17, 4, 1 },
  { ',', 0, "a,b,c,d\n1,2,\"3\n4\"", (const char** const)expected_18, 4, 1 },

  /* Inspired by https://github.com/d3/d3-dsv/blob/master/test/csv-test.js */
  /* returns the expected objects */
  { ',', 0, "a,b,c\n1,2,3\n", (const char** const)expected_19, 3, 1 },
  /* does not strip whitespace */
  { ',', 0, "a,b,c\n 1, 2,3\n", (const char** const)expected_20, 3, 1 },
  /* quoted values */
  { ',', 0, "a,b,c\n\"1\",2,3", (const char** const)expected_21, 3, 1 },
  { ',', 0, "a,b,c\n\"1\",2,3\n", (const char** const)expected_22, 3, 1 },
  /* quoted values with quotes */
  { ',', 0, "a\n\"\"\"hello\"\"\"", (const char** const)expected_23, 1, 1 },
  /* quoted values with newlines */
  { ',', 0, "a\n\"new\nline\"", (const char** const)expected_24, 1, 1 },
  { ',', 0, "a\n\"new\rline\"", (const char** const)expected_25, 1, 1 },
  { ',', 0, "a\n\"new\r\nline\"", (const char** const)expected_26, 1, 1 },
  /* Unix newlines */
  { ',', 0, "a,b,c\n1,2,3\n4,5,\"6\"\n7,8,9", (const char** const)expected_27, 3, 3 },
  /* Mac newlines */
  { ',', 0, "a,b,c\r1,2,3\r4,5,\"6\"\r7,8,9", (const char** const)expected_28, 3, 3 },
  /* DOS newlines */
  { ',', 0, "a,b,c\r\n1,2,3\r\n4,5,\"6\"\r\n7,8,9", (const char** const)expected_29, 3, 3 },

  /* \ is not a special character by default */
  { ',', 0, "a,b,c,d\ncol 1.1,col 1.2\\,\"col 1.3\\\",\"col\n1.4\"", (const char** const)expected_30, 4, 1 },

  { ',', SV_OPTION_COMMENT_PREFIX, "a,b,c\n#this is a comment\ncat,sat,mat\n", (const char** const)expected_31, 3, 1 },
  { ',', SV_OPTION_SKIP_ROWS, "skip this row\na,b,c\ncat,sat,mat\n", (const char** const)expected_3, 3, 1 },

  { '\0', 0, NULL,           NULL,       0, 0 }
};


#define EXPECTED_HEADER_IX(col) (col)
#define EXPECTED_DATA_IX(row, col) (((1+row) * c->expected->columns_count) + col)


static sv_status_t
svtest_line_callback(sv *t, void *user_data, const char* line, size_t length)
{
  svtest_context *c = (svtest_context*)user_data;

  if(c->line)
    free(c->line);
  c->line = (char*)malloc(length + 1);
  if(c->line)
    memcpy(c->line, line, length + 1);

  /* This code always succeeds */
  return SV_STATUS_OK;
}



static sv_status_t
svtest_header_callback(sv *t, void *user_data,
                       char** fields, size_t *widths, size_t count)
{
  unsigned int column_i;
  svtest_context *c = (svtest_context*)user_data;

  c->columns_count = count;

  for(column_i = 0; column_i < count; column_i++) {
    const char* header = fields[column_i];
    unsigned int ix = EXPECTED_HEADER_IX(column_i);
    const char* expected_header = c->expected->expected[ix];

    if(strcmp(header, expected_header)) {
      fprintf(stderr,
              "%s: Test %d FAIL '%s' - got header '%s' expected '%s'\n",
              program, c->test_index, c->line, header, expected_header);
      c->header_errors++;
    }
  }

  return SV_STATUS_OK;
}


static sv_status_t
svtest_fields_callback(sv *t, void *user_data,
                       char** fields, size_t *widths, size_t count)
{
  unsigned int column_i;
  svtest_context *c = (svtest_context*)user_data;

  for(column_i = 0; column_i < count; column_i++) {
    const char* data = fields[column_i];
    unsigned int ix = EXPECTED_DATA_IX(c->rows_count, column_i);
    const char* expected_data = c->expected->expected[ix];
    unsigned int header_ix = EXPECTED_HEADER_IX(column_i);
    const char* expected_header = c->expected->expected[header_ix];

    if(!expected_data && data) {
      fprintf(stderr,
              "%s: Test %d FAIL '%s' row %d got %s value >>>%s<<< expected no data\n",
              program, c->test_index, c->line, c->rows_count,
              expected_header,
              data);
      c->data_errors++;
    } else if(strcmp(data, expected_data)) {
      fprintf(stderr,
              "%s: Test %d FAIL '%s' row %d got %s value >>>%s<<< expected >>>%s<<<\n",
              program, c->test_index, c->line, c->rows_count,
              expected_header,
              data, expected_data);
      c->data_errors++;
    }
  }

  c->rows_count++;

  /* This code always succeeds */
  return SV_STATUS_OK;
}


static int
svtest_run_test(unsigned int test_index)
{
  svtest_context c;
  size_t data_len;
  sv *t = NULL;
  const svtest_data_set *test = &svtest_data[test_index];
  sv_status_t status;
  int rc = 0;

  memset(&c, '\0', sizeof(c));
  c.test_index = test_index;
  c.columns_count = 0;
  c.rows_count = 0;
  c.expected = test;
  c.line = NULL;

  data_len = strlen(test->data);

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, test->sep);
  if(!t) {
    fprintf(stderr, "%s: Failed to init SV library", program);
    rc = 1;
    return rc;
  }

  sv_set_option(t, SV_OPTION_LINE_CALLBACK, svtest_line_callback);

  if(test->option != 0) {
    sv_option_t opt = (sv_option_t)test->option;
    switch(opt) {
      case SV_OPTION_COMMENT_PREFIX:
        sv_set_option(t, opt, "#");
        break;

      case SV_OPTION_STRIP_WHITESPACE:
      case SV_OPTION_SKIP_ROWS:
        sv_set_option(t, opt, 1L);
        break;

      default:
        fprintf(stderr, "%s: Test %d ignoring unknown option %d\n",
                program, test_index, test->option);
    }
  }

  status = sv_parse_chunk(t, (char*)test->data, data_len);
  if(status != SV_STATUS_OK) {
    fprintf(stderr, "%s: Test %d FAIL - sv_parse_chunk() returned %d\n",
            program, test_index, (int)status);
    rc = 1;
    goto end_test;
  }

  status = sv_parse_chunk(t, NULL, 0);
  if(status != SV_STATUS_OK) {
      fprintf(stderr, "%s: Test %d FAIL - final sv_parse_chunk() returned %d\n",
              program, test_index, (int)status);
      rc = 1;
      goto end_test;
  }

  if(c.header_errors) {
    fprintf(stderr, "%s: Test %d FAIL '%s' - header errors\n",
            program, test_index, test->data);
    rc = 1;
  } else if(c.data_errors) {
    fprintf(stderr, "%s: Test %d FAIL '%s' - data errors\n",
            program, test_index, test->data);
    rc = 1;
  } else if(test->rows_count != c.rows_count) {
    fprintf(stderr, "%s: Test %d FAIL '%s' - saw %d records - expected %d\n",
            program, test_index, test->data, c.rows_count, test->rows_count); 
    rc = 1;
  } else {
    fprintf(stderr, "%s: Test %d OK\n",
            program, test_index);
  }

  end_test:
  if(c.line)
    free(c.line);

  sv_free(t);

  return rc;
}


#define MAX_TEST_INDEX (N_TESTS-1)

int
main(int argc, char *argv[])
{
  char *p;
  int rc = 0;
  unsigned int test_index;

  program = argv[0];
  if((p = strrchr(program, '/')))
    program = p + 1;
  else if((p = strrchr(program, '\\')))
    program = p + 1;
  argv[0] = program;

  if(argc < 1 || argc > 2) {
    fprintf(stderr, "USAGE: %s [TEST-INDEX 1..%d]\n", program, MAX_TEST_INDEX);
    rc = 1;
    goto tidy;
  }

  if(argc == 2) {
    test_index = atoi(argv[1]);
    if(test_index < 1 || test_index > MAX_TEST_INDEX) {
      fprintf(stderr, "%s: Test arg '%s' not in range 1..%d\n", program,
              argv[1], MAX_TEST_INDEX);
      rc = 1;
      goto tidy;
    }
    rc = svtest_run_test(test_index);
  } else {
    /* run all tests */

    for(test_index = 0; test_index < N_TESTS; test_index++) {
      int test_rc;

      test_rc = svtest_run_test(test_index);
      if(test_rc > 0) {
        /* error */
        rc++;
      } else if (test_rc < 0) {
        /* failure; do not run any more tests */
        rc = 1;
        break;
      }
    }
  }

 tidy:

  return rc;
}
