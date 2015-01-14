SV Parser Design
================

Goals:
* work character by character
* support [Model for Tabular Data and Metadata on the Web][1]
  from W3C [CSV on the Web Working Group][2]
* Handle EOL in fields: existing parser fails this


State diagram
=============


                     Table: list of rows  Row: list of cells    Cell: string
                            encoding           comment string         row number
                                               row number             column number
         +=======+
         ||START||
         +=======+
             |                                       +-------+                 +------------------+
             |                                       |       |Delim            |                  |
             v                                       v       +(empty field)    v                  +
     +----------------+    +---------------+       +-----------+       +---------------+   +---------------+
     |Start file      |    |Start record   |No EOL |Start field| Quote |In Quoted Field|   |Esc in quoted  |
     |----------------|    |---------------|------>|-----------|>----->|---------------|ESC|---------------|
     |Look for BOM    |--->|               | #     |           | else  |trim ws        |+->|               |
     |and set encoding|    |               |>--+   |           |>---+  |               |   |               |
     +----------------+    +--+------------+   |   +-----------+    |  +---------------+   +---------------+
                              |    ^  ^        |      EOL v         |
                           EOL|    |  |Not EOL |    (empty|field)   |          +------------------+
                              |    +----------------------|         |          |                  |
                              v       |        |                    |          v                  |
                           +----------+----+   |   +------------+   |  +--------------+    +------+--------+
                      +--->|End of Line    |   |   |Comment     |   |  |In Field      |    |Esc in field   |
                      |    |---------------|   +-->|------------|   +->|--------------|ESC |---------------|
                      |EOL |line++         |   EOL |Save content|      |trim ws       |+-->|               |
                      +----|               |<------|            |      |              |    |               |
                           +---------------+       +------------+      +--------------+    +---------------+


Implementation
==============

* State machine
* Try not to add too many new "classes"

Code sketch of data model and flags.

    typedef enum  {
      SV_STATE_UNKNOWN,
      /* After a reset and before any potential BOM */
      SV_STATE_START_FILE,
      /* After any BOM and expecting record start */
      SV_STATE_START_RECORD,
      /* Accepted \r or \n - new line; handle \r\n as 1 EOL */
      SV_STATE_EOL,
      /* Accepted # - read a commented row up to EOL */
      SV_STATE_COMMENT,
      /* Starting a cell */
      SV_STATE_IN_CELL,
      /* Accepted escape-char in a cell */
      SV_STATE_ESC_IN_CELL,
      /* Accepted quote-char starting a cell */
      SV_STATE_IN_QUOTED_CELL,
      /* Accepted escape-char in a quoted cell */
      SV_STATE_ESC_IN_QUOTED_CELL,
    } sv_parse_state;
    
    
    typedef struct {
      /* string, "" for empty cell (@len = 0) or NULL for null (@len = 0) */
      unsigned char* value;
      /* size of string above or NULL for null value */
      size_t len;
      /* this column was skipped */
      unsigned int is_skipped:1;
      /* content flags */
      unsigned int contains_quote:1;
      unsigned int contains_delimiter:1;
      /* whitespace (' ', \t, \v) content flags */
      unsigned int contains_leading_ws:1;
      unsigned int contains_trailing_ws:1;
      /* if the delimiter is ws, will be same as @contains_delimiter */
      unsigned int contains_contains_ws:1;
      /* eol content flag */
      unsigned int contains_eol:1; /* \r or \n */
    } sv_cell;
    
    
    typedef struct {
      /* number of cells in this row */
      size_t size;
      /* array of cells size @size */
      sv_cell* cells;
      /* header row;  */
      unsigned int is_header:1;
      /* comment: @width = 1 and @cells[0] is the comment cell */
      unsigned int is_comment:1;
      /* this row was skipped */
      unsigned int is_skipped:1;
      /* this row had no EOL; file ended */
      unsigned int no_eol:1;
    } sv_row;
    
    
    typedef struct {
      /* number of rows in this table */
      size_t size;
      /* array of rows size @size */
      sv_row* rows;
    } sv_table;
    
    
    typedef struct {
      const char* encoding; /* default "utf-8" */
      const char eol; /* '\r', '\n' or '\0' for CRLF (default) */
      const char quote; /* default '"' */
      const char escape; /* default '"' */
      unsigned int skip_rows; /* default 0 */
      const char comment_prefix; /* default '# '*/
      unsigned int header_row_count; /* default 1 */
      const char delimiter; /* default ',' */
      unsigned int skip_columns; /* default 0 */
      unsigned int header_column_count; /* default 0 */
      unsigned int skip_blank_rows:1; /* default 0 (false) */
      unsigned int trim:2; /* 0 (false), 1 (start), 2 (end), 3 (both / true) */
    } sv_parse;


[1]: http://www.w3.org/TR/tabular-data-model/
[2]: http://www.w3.org/2013/csvw/wiki/Main_Page
