# libsv Design

This document outlines the design and architecture of the `libsv` library.

## Core Principles

The library is designed around the following core principles:

* **Character-by-Character Processing:** The parser operates on a stream of characters, making it suitable for a wide range of input sources.
* **Memory Efficiency:** It uses a callback-based API, which avoids loading the entire file into memory. This allows it to handle very large files with a small memory footprint.
* **Flexibility:** The library provides a rich set of options to control parsing behavior, including custom delimiters, quoting, and error handling.

## Parsing Logic

The core of the parser is a state machine that processes input one character at a time. This approach allows it to correctly handle complex cases like quoted fields, escaped characters, and newlines within fields.

### State Diagram

The following diagram illustrates the basic flow of the parser's state machine:

```ascii
                 Table: list of rows  Row: list of cells    Cell: string
                        encoding           comment string         row number
                                           row number             column number
     +=======+
     ||START||
     +=======+
         |
         |                                       +-------+                 +------------------+
         |                                       |       |Delim            |                  |
         v                                       v       +(empty field)    v                  +
 +----------------+    +---------------+       +-----------+       +---------------+   +---------------+ 
 |Start file      |    |Start record   |No EOL |Start field| Quote |In Quoted Field|   |Esc in quoted  |
 |----------------|    |---------------|------>|-----------|>----->|---------------|ESC|---------------|
 |Look for BOM    |--->|               | #     |           | else  |trim ws        |+->|               |
 |and set encoding|    |               |>--+   |           |>---+  |               |   |               |
 +----------------+    +--+------------+   |   +-----------+    |  +---------------+   +---------------+ 
                          |    ^  ^ ^      |      EOL v         |
                        \r|    |  | +---------+ (empty|field)   |          +------------------+
                          |    +--------------|-------|         |          |                  |
                          v       |else    |  |else             |          v                  |
                       +----------+----+   |  |+------------+   |  +--------------+    +------+--------+
                  +--->|CR             |   |  +|Comment     |   |  |In Field      |    |Esc in field   |
                  |    |---------------|   |   |------------|   +->|--------------|ESC |---------------|
                  |  \r|Handle \r\n as |   +-->|Save content|      |trim ws       |+-->|               |
                  +----|one line       |<------|            |      |              |    |               |
                       +---------------+    \r +------------+      +--------------+    +---------------+ 
```

## Key Features

### Callback-based API

Instead of returning a fully parsed data structure, `libsv` uses callbacks to deliver data to the application. The user provides function pointers for handling header rows and data rows. This approach is highly memory-efficient and allows the application to process data as it is being parsed.

### Null Value Handling

The library provides flexible and configurable null value detection:

* **Automatic Detection:** By default, it recognizes common representations of null values, such as empty fields (`,,`), quoted empty fields (`,"",`), and common markers like `NA` and `NULL`.
* **Custom Configuration:** The `SV_OPTION_NULL_VALUES` option allows users to specify a custom list of strings that should be treated as null.
* **Enhanced Null Handling:** The `SV_OPTION_NULL_HANDLING` option changes the behavior to return `NULL` pointers for null fields, allowing the application to distinguish between an empty string and a missing value.

### Security

The library provides a field size limit to prevent denial-of-service attacks from malicious CSV files with extremely large fields. The default limit is 128KB and can be configured using the `SV_OPTION_FIELD_SIZE_LIMIT` option. When the limit is exceeded, the parser returns an `SV_STATUS_FIELD_TOO_LARGE` error.

## API

See the `sv.h` header file for the full public API documentation.
