libsv - a simple Separated Values (CSV, TSV) library in ANSI C
===============================================================

License: LGPL 2.1+ or GPL 2+ or Apache 2+

Home: <https://github.com/dajobe/libsv/tree/master>
Source: git clone git://github.com/dajobe/libsv.git

Features
--------

* CSV and TSV parsing with configurable delimiters
* Configurable null value handling for missing data
* Support for quoted fields and custom quote characters
* Comment line handling
* Row skipping and header management
* Whitespace trimming options
* Memory-safe parsing with overflow protection

## Null Value Handling

The library automatically detects common null representations:
* Empty fields (`,,`)
* Empty quoted fields (`,""`)
* Standard null markers (`\N`, `NA`, `NULL`)

Custom null values can be configured:
```c
const char* nulls[] = {"", "NA", "NULL", "\\N", "missing", "unknown"};
sv_set_option(t, SV_OPTION_NULL_VALUES, nulls, 6);
```

### Null Handling Modes

**Default Mode** (backward compatible): Null values are returned as empty strings `""` to preserve existing behavior.

**Enhanced Mode**: Enable with `SV_OPTION_NULL_HANDLING` to return `NULL` pointers for missing data, allowing clear distinction between empty strings and missing values:

```c
/* Enable enhanced null handling */
sv_set_option(t, SV_OPTION_NULL_HANDLING, 1L);

/* Now null values return NULL pointers instead of empty strings */
/* This allows callbacks to distinguish between "" and missing data */
```

Standalone build
----------------

```shell
    make -f GNUMakefile
```

Embedded build in an automake package
-------------------------------------

``` shell
    git submodule add --name libsv https://github.com/dajobe/libsv.git libsv
    git submodule init libsv
    git submodule update libsv
```

Then add these lines to your library `Makefile.am`

``` Makefile
    AM_CFLAGS += -DSV_CONFIG -I$(top_srcdir)/libsv
    libexample_la_LIBADD += $(top_builddir)/libsv/libsv.la
    libexample_la_DEPENDENCIES += $(top_builddir)/libsv/libsv.la

    $(top_builddir)/libsv/libsv.la:
    <tab>cd $(top_builddir)/libsv && $(MAKE) libsv.la
```

and add a configuration file `sv_config.h` in the include path which
defines HAVE_STDLIB_H etc. as needed by sv.h and sv.c

Optionally you might want in this file to redefine the exposed API
symbols with lines like:

``` c
    #define sv_new example_sv_new
    #define sv_free example_sv_free
    #define sv_reset example_sv_reset
    #define sv_set_option example_sv_set_option
    #define sv_get_line example_sv_get_line
    #define sv_get_header example_sv_get_header
    #define sv_parse_chunk example_sv_parse_chunk
    #define sv_write_fields example_sv_write_fields
```

You can see this demonstrated inside [Rasqal](https://github.com/dajobe/rasqal)

Example
-------

See `example.c` for API use.

Dave Beckett
<https://www.dajobe.org/>
