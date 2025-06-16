/* -*- Mode: c; c-basic-offset: 2 -*-
 *
 * svtest.c - SV tests
 *
 * Copyright (C) 2014-2025, Dave Beckett http://www.dajobe.org/
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
static const char* expected_custom_A1[] = {"h"};
static const char* expected_custom_B2[] = {"h", "x'y", "z"};
static const char* expected_custom_B7[] = {"hdr1", "hdr2", "\"a", "b\"", "c"};
static const char* expected_custom_delimiters_only[] = {"", "", "", ""};
static const char* expected_custom_ends_with_delimiter[] = {"header1", "header2", ""};
static const char* expected_custom_crlf_eol[] = {"h1", "h2", "d1", "d2"};
static const char* expected_custom_strip_ws_empty[] = {"h1", "h2", "h3", "", "", "xyz"};
static const char* expected_custom_skip_comment_interaction[] = {"Header1", "Header2", "Data1", "Data2"};


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


static int svtest_run_custom_A1(void);
static int svtest_run_custom_B2(void);
static int svtest_run_custom_B7(void);
static int svtest_run_custom_empty_file(void);
static int svtest_run_custom_newlines_only_file(void);
static int svtest_run_custom_delimiters_only_file(void);
static int svtest_run_custom_ends_with_delimiter(void);
static int svtest_run_custom_crlf_eol(void);
static int svtest_run_custom_strip_whitespace_empty(void);
static int svtest_run_custom_skip_comment_interaction(void);
static int svtest_run_custom_skip_rows_gt_total(void);


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

      case SV_OPTION_NONE:
      case SV_OPTION_SAVE_HEADER:
      case SV_OPTION_BAD_DATA_ERROR:
      case SV_OPTION_QUOTED_FIELDS:
      case SV_OPTION_QUOTE_CHAR:
      case SV_OPTION_LINE_CALLBACK:
      case SV_OPTION_DOUBLE_QUOTE:
      case SV_OPTION_ESCAPE_CHAR:
      case SV_OPTION_COMMENT_CALLBACK:
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


static int svtest_run_custom_skip_rows_gt_total(void) {
  svtest_context c;
  sv *t = NULL;
  svtest_data_set current_test_data = {
      .sep = ',',
      .option = 0, /* Options set manually */
      .data = "Header1,Header2\nData1,Data2",
      .expected = NULL,
      .columns_count = 0, /* Expected header columns (none) */
      .rows_count = 0     /* Expected data rows (none) */
  };
  int rc = 0;

  memset(&c, '\0', sizeof(c));
  c.test_index = -502; /* Unique ID */
  c.expected = &current_test_data;

  fprintf(stderr, "Running Custom Test Skip Rows > Total Lines...\n");

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, current_test_data.sep);
  if (!t) {
    fprintf(stderr, "%s: Test Custom Skip Rows > Total Lines FAIL - sv_new() failed\n", program);
    return 1;
  }

  sv_set_option(t, SV_OPTION_SKIP_ROWS, 3L); /* Skip 3 rows */

  sv_parse_chunk(t, (char*)current_test_data.data, strlen(current_test_data.data));
  sv_parse_chunk(t, NULL, 0);

  if (c.header_errors != 0) {
    fprintf(stderr, "%s: Test Custom Skip Rows > Total Lines FAIL - header errors (%d)\n", program, c.header_errors);
    rc = 1;
  }
  if (c.data_errors != 0) {
    fprintf(stderr, "%s: Test Custom Skip Rows > Total Lines FAIL - data errors (%d)\n", program, c.data_errors);
    rc = 1;
  }
  /* c.columns_count is from header_callback, should be 0 */
  if (c.columns_count != current_test_data.columns_count) {
    fprintf(stderr, "%s: Test Custom Skip Rows > Total Lines FAIL - got %d header columns, expected %d\n", program, c.columns_count, current_test_data.columns_count);
    rc = 1;
  }
  /* c.rows_count is from fields_callback, should be 0 */
  if (c.rows_count != current_test_data.rows_count) {
     fprintf(stderr, "%s: Test Custom Skip Rows > Total Lines FAIL - saw %d data records, expected %d\n", program, c.rows_count, current_test_data.rows_count);
     rc = 1;
  }

  if (rc == 0) {
    fprintf(stderr, "%s: Test Custom Skip Rows > Total Lines OK\n", program);
  }

  if (c.line) free(c.line);
  sv_free(t);

  return rc;
}


static int svtest_run_custom_skip_comment_interaction(void) {
  svtest_context c;
  sv *t = NULL;
  svtest_data_set current_test_data = {
      .sep = ',',
      .option = 0, /* Options set manually */
      .data = "# Comment line\nHeader1,Header2\nData1,Data2",
      .expected = expected_custom_skip_comment_interaction,
      .columns_count = 2, /* Expected header columns */
      .rows_count = 1     /* Expected data rows */
  };
  int rc = 0;

  memset(&c, '\0', sizeof(c));
  c.test_index = -501; /* Unique ID */
  c.expected = &current_test_data;

  fprintf(stderr, "Running Custom Test Skip/Comment Interaction...\n");

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, current_test_data.sep);
  if (!t) {
    fprintf(stderr, "%s: Test Custom Skip/Comment Interaction FAIL - sv_new() failed\n", program);
    return 1;
  }

  sv_set_option(t, SV_OPTION_COMMENT_PREFIX, "#");
  sv_set_option(t, SV_OPTION_SKIP_ROWS, 1L); /* Skip 1 row */

  sv_parse_chunk(t, (char*)current_test_data.data, strlen(current_test_data.data));
  sv_parse_chunk(t, NULL, 0);

  if (c.header_errors != 0) {
    fprintf(stderr, "%s: Test Custom Skip/Comment Interaction FAIL - header errors (%d)\n", program, c.header_errors);
    rc = 1;
  }
  if (c.data_errors != 0) {
    fprintf(stderr, "%s: Test Custom Skip/Comment Interaction FAIL - data errors (%d)\n", program, c.data_errors);
    rc = 1;
  }
  if (c.columns_count != current_test_data.columns_count) {
    fprintf(stderr, "%s: Test Custom Skip/Comment Interaction FAIL - got %d header columns, expected %d\n", program, c.columns_count, current_test_data.columns_count);
    rc = 1;
  }
  if (c.rows_count != current_test_data.rows_count) {
     fprintf(stderr, "%s: Test Custom Skip/Comment Interaction FAIL - saw %d data records, expected %d\n", program, c.rows_count, current_test_data.rows_count);
     rc = 1;
  }

  if (rc == 0) {
    fprintf(stderr, "%s: Test Custom Skip/Comment Interaction OK\n", program);
  }

  if (c.line) free(c.line);
  sv_free(t);

  return rc;
}


static int svtest_run_custom_strip_whitespace_empty(void) {
  svtest_context c;
  sv *t = NULL;
  svtest_data_set current_test_data = {
      .sep = ',',
      /* SV_OPTION_STRIP_WHITESPACE is set via sv_set_option directly in this test */
      .option = 0,
      .data = "h1,h2,h3\n\"   \",\"  \",xyz",
      .expected = expected_custom_strip_ws_empty,
      .columns_count = 3, /* Expected header columns */
      .rows_count = 1     /* Expected data rows */
  };
  int rc = 0;

  memset(&c, '\0', sizeof(c));
  c.test_index = -401; /* Unique ID */
  c.expected = &current_test_data;

  fprintf(stderr, "Running Custom Test Strip Whitespace to Empty...\n");

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, current_test_data.sep);
  if (!t) {
    fprintf(stderr, "%s: Test Custom Strip Whitespace to Empty FAIL - sv_new() failed\n", program);
    return 1;
  }

  /* Enable stripping whitespace */
  sv_set_option(t, SV_OPTION_STRIP_WHITESPACE, 1L);

  sv_parse_chunk(t, (char*)current_test_data.data, strlen(current_test_data.data));
  sv_parse_chunk(t, NULL, 0); /* Finalize parsing */

  if (c.header_errors != 0) {
    fprintf(stderr, "%s: Test Custom Strip Whitespace to Empty FAIL - header errors (%d)\n", program, c.header_errors);
    rc = 1;
  }
  if (c.data_errors != 0) {
    fprintf(stderr, "%s: Test Custom Strip Whitespace to Empty FAIL - data errors (%d)\n", program, c.data_errors);
    rc = 1;
  }
  if (c.columns_count != current_test_data.columns_count) {
    fprintf(stderr, "%s: Test Custom Strip Whitespace to Empty FAIL - got %d header columns, expected %d\n", program, c.columns_count, current_test_data.columns_count);
    rc = 1;
  }
  if (c.rows_count != current_test_data.rows_count) {
     fprintf(stderr, "%s: Test Custom Strip Whitespace to Empty FAIL - saw %d data records, expected %d\n", program, c.rows_count, current_test_data.rows_count);
     rc = 1;
  }

  if (rc == 0) {
    fprintf(stderr, "%s: Test Custom Strip Whitespace to Empty OK\n", program);
  }

  if (c.line) free(c.line);
  sv_free(t);

  return rc;
}


static int svtest_run_custom_crlf_eol(void) {
  svtest_context c;
  sv *t = NULL;
  svtest_data_set current_test_data = {
      .sep = ',',
      .option = 0,
      .data = "h1,h2\r\nd1,d2", /* Input with CRLF */
      .expected = expected_custom_crlf_eol,
      .columns_count = 2, /* Expected header columns */
      .rows_count = 1     /* Expected data rows */
  };
  int rc = 0;

  memset(&c, '\0', sizeof(c));
  c.test_index = -301; /* Unique ID */
  c.expected = &current_test_data;

  fprintf(stderr, "Running Custom Test CRLF EOL...\n");

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, current_test_data.sep);
  if (!t) {
    fprintf(stderr, "%s: Test Custom CRLF EOL FAIL - sv_new() failed\n", program);
    return 1;
  }

  sv_parse_chunk(t, (char*)current_test_data.data, strlen(current_test_data.data));
  sv_parse_chunk(t, NULL, 0); /* Finalize parsing */

  if (c.header_errors != 0) {
    fprintf(stderr, "%s: Test Custom CRLF EOL FAIL - header errors (%d)\n", program, c.header_errors);
    rc = 1;
  }
  if (c.data_errors != 0) {
    fprintf(stderr, "%s: Test Custom CRLF EOL FAIL - data errors (%d)\n", program, c.data_errors);
    rc = 1;
  }
  if (c.columns_count != current_test_data.columns_count) {
    fprintf(stderr, "%s: Test Custom CRLF EOL FAIL - got %d header columns, expected %d\n", program, c.columns_count, current_test_data.columns_count);
    rc = 1;
  }
  if (c.rows_count != current_test_data.rows_count) {
     fprintf(stderr, "%s: Test Custom CRLF EOL FAIL - saw %d data records, expected %d\n", program, c.rows_count, current_test_data.rows_count);
     rc = 1;
  }

  if (rc == 0) {
    fprintf(stderr, "%s: Test Custom CRLF EOL OK\n", program);
  }

  if (c.line) free(c.line);
  sv_free(t);

  return rc;
}


static int svtest_run_custom_ends_with_delimiter(void) {
  svtest_context c;
  sv *t = NULL;
  svtest_data_set current_test_data = {
      .sep = ',',
      .option = 0,
      .data = "header1,header2,", /* Input ends with a delimiter */
      .expected = expected_custom_ends_with_delimiter,
      .columns_count = 3, /* Expected header columns */
      .rows_count = 0     /* Expected data rows */
  };
  int rc = 0;

  memset(&c, '\0', sizeof(c));
  c.test_index = -204; /* Unique ID */
  c.expected = &current_test_data;

  fprintf(stderr, "Running Custom Test Ends With Delimiter...\n");

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, current_test_data.sep);
  if (!t) {
    fprintf(stderr, "%s: Test Custom Ends With Delimiter FAIL - sv_new() failed\n", program);
    return 1;
  }

  sv_parse_chunk(t, (char*)current_test_data.data, strlen(current_test_data.data));
  sv_parse_chunk(t, NULL, 0); /* Finalize parsing */

  if (c.header_errors != 0) {
    fprintf(stderr, "%s: Test Custom Ends With Delimiter FAIL - header errors (%d)\n", program, c.header_errors);
    rc = 1;
  }
  if (c.data_errors != 0) {
    fprintf(stderr, "%s: Test Custom Ends With Delimiter FAIL - data errors (%d)\n", program, c.data_errors);
    rc = 1;
  }
  if (c.columns_count != current_test_data.columns_count) {
    fprintf(stderr, "%s: Test Custom Ends With Delimiter FAIL - got %d header columns, expected %d\n", program, c.columns_count, current_test_data.columns_count);
    rc = 1;
  }
  if (c.rows_count != current_test_data.rows_count) {
     fprintf(stderr, "%s: Test Custom Ends With Delimiter FAIL - saw %d data records, expected %d\n", program, c.rows_count, current_test_data.rows_count);
     rc = 1;
  }

  if (rc == 0) {
    fprintf(stderr, "%s: Test Custom Ends With Delimiter OK\n", program);
  }

  if (c.line) free(c.line);
  sv_free(t);

  return rc;
}


static int svtest_run_custom_delimiters_only_file(void) {
  svtest_context c;
  sv *t = NULL;
  /* Initialize current_test_data at declaration */
  svtest_data_set current_test_data = {
      .sep = ',',
      .option = 0,
      .data = ",,,", /* Input: three commas, implying four empty fields */
      .expected = expected_custom_delimiters_only,
      .columns_count = 4, /* Expected header columns (all empty) */
      .rows_count = 0     /* Expected data rows (no data lines) */
  };
  int rc = 0;

  /* Initialize context */
  memset(&c, '\0', sizeof(c));
  c.test_index = -203; /* Unique ID for this custom test */
  c.expected = &current_test_data;

  fprintf(stderr, "Running Custom Test Delimiters Only File...\n");

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, current_test_data.sep);
  if (!t) {
    fprintf(stderr, "%s: Test Custom Delimiters Only File FAIL - sv_new() failed\n", program);
    return 1;
  }

  /* Parse data */
  sv_parse_chunk(t, (char*)current_test_data.data, strlen(current_test_data.data));
  sv_parse_chunk(t, NULL, 0); /* Finalize parsing */

  /* Check results */
  if (c.header_errors != 0) {
    fprintf(stderr, "%s: Test Custom Delimiters Only File FAIL - header errors (%d)\n", program, c.header_errors);
    rc = 1;
  }
  if (c.data_errors != 0) {
    fprintf(stderr, "%s: Test Custom Delimiters Only File FAIL - data errors (%d)\n", program, c.data_errors);
    rc = 1;
  }
  /* c.columns_count is set by header_callback. */
  if (c.columns_count != current_test_data.columns_count) {
    fprintf(stderr, "%s: Test Custom Delimiters Only File FAIL - got %d header columns, expected %d\n", program, c.columns_count, current_test_data.columns_count);
    rc = 1;
  }
  /* c.rows_count is set by fields_callback. */
  if (c.rows_count != current_test_data.rows_count) {
     fprintf(stderr, "%s: Test Custom Delimiters Only File FAIL - saw %d data records, expected %d\n", program, c.rows_count, current_test_data.rows_count);
     rc = 1;
  }

  if (rc == 0) {
    fprintf(stderr, "%s: Test Custom Delimiters Only File OK\n", program);
  }

  if (c.line) free(c.line);
  sv_free(t);

  return rc;
}


static int svtest_run_custom_newlines_only_file(void) {
  svtest_context c;
  sv *t = NULL;
  /* Initialize current_test_data at declaration */
  svtest_data_set current_test_data = {
      .sep = ',',
      .option = 0,
      .data = "\n\n\n", /* Input: three newline characters */
      .expected = NULL,
      .columns_count = 0, /* Expected header columns */
      .rows_count = 0     /* Expected data rows (empty lines are not data rows) */
  };
  int rc = 0;

  /* Initialize context */
  memset(&c, '\0', sizeof(c));
  c.test_index = -202; /* Unique ID for this custom test */
  c.expected = &current_test_data;

  fprintf(stderr, "Running Custom Test Newlines Only File...\n");

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, current_test_data.sep);
  if (!t) {
    fprintf(stderr, "%s: Test Custom Newlines Only File FAIL - sv_new() failed\n", program);
    return 1;
  }

  /* Parse data */
  sv_parse_chunk(t, (char*)current_test_data.data, strlen(current_test_data.data));
  sv_parse_chunk(t, NULL, 0); /* Finalize parsing */

  /* Check results */
  if (c.header_errors != 0) {
    fprintf(stderr, "%s: Test Custom Newlines Only File FAIL - header errors (%d)\n", program, c.header_errors);
    rc = 1;
  }
  if (c.data_errors != 0) {
    fprintf(stderr, "%s: Test Custom Newlines Only File FAIL - data errors (%d)\n", program, c.data_errors);
    rc = 1;
  }
  if (c.columns_count != current_test_data.columns_count) {
    fprintf(stderr, "%s: Test Custom Newlines Only File FAIL - got %d header columns, expected %d\n", program, c.columns_count, current_test_data.columns_count);
    rc = 1;
  }
  if (c.rows_count != current_test_data.rows_count) {
     fprintf(stderr, "%s: Test Custom Newlines Only File FAIL - saw %d data records, expected %d\n", program, c.rows_count, current_test_data.rows_count);
     rc = 1;
  }

  if (rc == 0) {
    fprintf(stderr, "%s: Test Custom Newlines Only File OK\n", program);
  }

  if (c.line) free(c.line);
  sv_free(t);

  return rc;
}


static int svtest_run_custom_empty_file(void) {
  svtest_context c;
  sv *t = NULL;
  /* Initialize current_test_data at declaration */
  svtest_data_set current_test_data = {
      .sep = ',',
      .option = 0,
      .data = "", /* Empty string for empty file */
      .expected = NULL, /* Callbacks should not be invoked for header/data */
      .columns_count = 0, /* Expected header columns */
      .rows_count = 0     /* Expected data rows */
  };
  int rc = 0;

  /* Initialize context */
  memset(&c, '\0', sizeof(c));
  c.test_index = -201; /* Unique ID for this custom test */
  c.expected = &current_test_data; /* Point to our specific configuration */

  fprintf(stderr, "Running Custom Test Empty File...\n");

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, current_test_data.sep);
  if (!t) {
    fprintf(stderr, "%s: Test Custom Empty File FAIL - sv_new() failed\n", program);
    return 1;
  }

  /* Parse data (empty) */
  sv_parse_chunk(t, (char*)current_test_data.data, strlen(current_test_data.data));
  sv_parse_chunk(t, NULL, 0); /* Finalize parsing */

  /* Check results */
  if (c.header_errors != 0) {
    fprintf(stderr, "%s: Test Custom Empty File FAIL - header errors (%d)\n", program, c.header_errors);
    rc = 1;
  }
  if (c.data_errors != 0) {
    fprintf(stderr, "%s: Test Custom Empty File FAIL - data errors (%d)\n", program, c.data_errors);
    rc = 1;
  }
  /* c.columns_count is set by header_callback. If never called, it remains 0. */
  if (c.columns_count != current_test_data.columns_count) {
    fprintf(stderr, "%s: Test Custom Empty File FAIL - got %d header columns, expected %d\n", program, c.columns_count, current_test_data.columns_count);
    rc = 1;
  }
  /* c.rows_count is set by fields_callback. If never called, it remains 0. */
  if (c.rows_count != current_test_data.rows_count) {
     fprintf(stderr, "%s: Test Custom Empty File FAIL - saw %d data records, expected %d\n", program, c.rows_count, current_test_data.rows_count);
     rc = 1;
  }

  if (rc == 0) {
    fprintf(stderr, "%s: Test Custom Empty File OK\n", program);
  }

  if (c.line) free(c.line); /* c.line might be allocated by svtest_line_callback if used */
  sv_free(t);

  return rc;
}


static int svtest_run_custom_B7(void) {
  svtest_context c;
  sv *t = NULL;
  /* Initialize current_test_data at declaration */
  svtest_data_set current_test_data = {
      .sep = ',',
      .option = 0, /* Options are set manually below */
      .data = "hdr1,hdr2\n\"a,b\",c", /* Corrected Input: "a,b" (literal quotes), c */
      .expected = expected_custom_B7,
      .columns_count = 2, /* Expected header columns ("hdr1", "hdr2") */
      .rows_count = 1     /* Expected data rows (one row: ""a,b"", "c") */
  };
  int rc = 0;

  /* Initialize context */
  memset(&c, '\0', sizeof(c));
  c.test_index = -107; /* Unique ID for this custom test */
  c.expected = &current_test_data;

  fprintf(stderr, "Running Custom Test B7 (Quote Char NUL - quoting disabled)...\n");

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, current_test_data.sep);
  if (!t) {
    fprintf(stderr, "%s: Test Custom B7 FAIL - sv_new() failed\n", program);
    return 1;
  }

  /* Set custom option: Disable quoting */
  sv_set_option(t, SV_OPTION_QUOTE_CHAR, '\0');

  /* Parse data */
  sv_parse_chunk(t, (char*)current_test_data.data, strlen(current_test_data.data));
  sv_parse_chunk(t, NULL, 0); /* Finalize parsing */

  /* Check results */
  if (c.header_errors != 0) {
    fprintf(stderr, "%s: Test Custom B7 FAIL - header errors (%d)\n", program, c.header_errors);
    rc = 1;
  }
  if (c.data_errors != 0) {
    fprintf(stderr, "%s: Test Custom B7 FAIL - data errors (%d)\n", program, c.data_errors);
    rc = 1;
  }
  if (c.columns_count != current_test_data.columns_count) {
    fprintf(stderr, "%s: Test Custom B7 FAIL - got %d header columns, expected %d\n", program, c.columns_count, current_test_data.columns_count);
    rc = 1;
  }
  if (c.rows_count != current_test_data.rows_count) {
     fprintf(stderr, "%s: Test Custom B7 FAIL - saw %d data records, expected %d\n", program, c.rows_count, current_test_data.rows_count);
     rc = 1;
  }

  if (rc == 0) {
    fprintf(stderr, "%s: Test Custom B7 OK\n", program);
  }

  if (c.line) free(c.line);
  sv_free(t);

  return rc;
}


static int svtest_run_custom_B2(void) {
  svtest_context c;
  sv *t = NULL;
  int rc = 0;
  /* Test data: header 'h', then one row with two fields: "x'y" and "z" */
  /* Quoted with ' and ' used as literal via '' */
  const char* test_data_str = "h\n'x''y','z'";

  svtest_data_set current_test_data = {
      .sep = ',',
      .option = 0, /* Options are set manually below */
      .data = test_data_str,
      .expected = expected_custom_B2,
      .columns_count = 1, /* Expected header columns ("h") */
      .rows_count = 1     /* Expected data rows (one row: "x'y", "z") */
  };

  /* Initialize context */
  memset(&c, '\0', sizeof(c));
  c.test_index = -102; /* Unique ID for this custom test */
  c.expected = &current_test_data;

  fprintf(stderr, "Running Custom Test B2 (Custom Quote ', Double Quote ON)...\n");

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, current_test_data.sep);
  if (!t) {
    fprintf(stderr, "%s: Test Custom B2 FAIL - sv_new() failed\n", program);
    return 1;
  }

  /* Set custom options */
  sv_set_option(t, SV_OPTION_QUOTE_CHAR, '\'');
  sv_set_option(t, SV_OPTION_DOUBLE_QUOTE, 1L);

  /* Parse data */
  sv_parse_chunk(t, (char*)current_test_data.data, strlen(current_test_data.data));
  sv_parse_chunk(t, NULL, 0); /* Finalize parsing */

  /* Check results */
  if (c.header_errors != 0) {
    fprintf(stderr, "%s: Test Custom B2 FAIL - header errors (%d)\n", program, c.header_errors);
    rc = 1;
  }
  if (c.data_errors != 0) {
    fprintf(stderr, "%s: Test Custom B2 FAIL - data errors (%d)\n", program, c.data_errors);
    rc = 1;
  }
  if (c.columns_count != current_test_data.columns_count) {
    fprintf(stderr, "%s: Test Custom B2 FAIL - got %d header columns, expected %d\n", program, c.columns_count, current_test_data.columns_count);
    rc = 1;
  }
  if (c.rows_count != current_test_data.rows_count) {
     fprintf(stderr, "%s: Test Custom B2 FAIL - saw %d data records, expected %d\n", program, c.rows_count, current_test_data.rows_count);
     rc = 1;
  }

  if (rc == 0) {
    fprintf(stderr, "%s: Test Custom B2 OK\n", program);
  }

  if (c.line) free(c.line);
  sv_free(t);

  return rc;
}


static int svtest_run_custom_A1(void) {
  svtest_context c;
  sv *t = NULL;
  int rc = 0;
  const char* test_data_str = "h\n\"abc"; /* Header "h", Data "abc" (unclosed quote) */

  svtest_data_set current_test_data = {
    .sep = ',',
    .option = 0,
    .data = test_data_str,
    .expected = expected_custom_A1,
    .columns_count = 1, /* Expected header columns */
    .rows_count = 0     /* Expected data rows: 0, as unclosed quote invalidates the row */
  };

  /* Initialize context */
  memset(&c, '\0', sizeof(c));
  c.test_index = -101; /* Unique ID for this custom test */
  c.expected = &current_test_data;

  fprintf(stderr, "Running Custom Test A1 (Unclosed Quote EOF)...\n");

  t = sv_new(&c, svtest_header_callback, svtest_fields_callback, current_test_data.sep);
  if (!t) {
    fprintf(stderr, "%s: Test Custom A1 FAIL - sv_new() failed\n", program);
    return 1;
  }

  /* Parse data */
  sv_parse_chunk(t, (char*)current_test_data.data, strlen(current_test_data.data));
  sv_parse_chunk(t, NULL, 0); /* Finalize parsing */

  /* Check results (callbacks modify c.header_errors, c.data_errors, c.columns_count, c.rows_count) */
  if (c.header_errors != 0) {
    fprintf(stderr, "%s: Test Custom A1 FAIL - header errors (%d)\n", program, c.header_errors);
    rc = 1;
  }
  if (c.data_errors != 0) {
    fprintf(stderr, "%s: Test Custom A1 FAIL - data errors (%d)\n", program, c.data_errors);
    rc = 1;
  }
  if (c.columns_count != current_test_data.columns_count) {
    fprintf(stderr, "%s: Test Custom A1 FAIL - got %d header columns, expected %d\n", program, c.columns_count, current_test_data.columns_count);
    rc = 1;
  }
  /* For data rows, svtest_fields_callback increments c.rows_count.
   * The check for actual number of rows against expected is
   * implicitly handled by svtest_fields_callback not being called
   * enough times / too many times.  Here we ensure the final count
   * matches.
  */
  if (c.rows_count != current_test_data.rows_count) {
     fprintf(stderr, "%s: Test Custom A1 FAIL - saw %d data records, expected %d\n", program, c.rows_count, current_test_data.rows_count);
     rc = 1;
  }


  if (rc == 0) {
    fprintf(stderr, "%s: Test Custom A1 OK\n", program);
  }

  if (c.line) free(c.line);
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
    /* Add the custom test call AFTER the loop */
    if (svtest_run_custom_A1() != 0) {
      rc++;
    }
    if (svtest_run_custom_B2() != 0) {
      rc++;
    }
    if (svtest_run_custom_B7() != 0) {
      rc++;
    }
    if (svtest_run_custom_empty_file() != 0) {
      rc++;
    }
    if (svtest_run_custom_newlines_only_file() != 0) {
      rc++;
    }
    if (svtest_run_custom_delimiters_only_file() != 0) {
      rc++;
    }
    if (svtest_run_custom_ends_with_delimiter() != 0) {
      rc++;
    }
    if (svtest_run_custom_crlf_eol() != 0) {
      rc++;
    }
    if (svtest_run_custom_strip_whitespace_empty() != 0) {
      rc++;
    }
    if (svtest_run_custom_skip_comment_interaction() != 0) {
      rc++;
    }
    if (svtest_run_custom_skip_rows_gt_total() != 0) {
      rc++;
    }
  }

 tidy:

  return rc;
}
