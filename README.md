libsv - a simple Separated Values (CSV, TSV) library in ANSI C.
===============================================================

License: LGPL 2.1+ or GPL 2+ or Apache 2+

Home: http://github.com/dajobe/libsv/tree/master
Source: git clone git://github.com/dajobe/libsv.git

Standalone build
----------------

    make -f GNUMakefile

Embedded build in an automake package
-------------------------------------

    $ git submodule add --name libsv https://github.com/dajobe/libsv.git libsv
    $ git submodule init libsv
    $ git submodule update libsv

Then add these lines to your library `Makefile.am`

    AM_CFLAGS += -DSV_CONFIG -I$(top_srcdir)/libsv
    libexample_la_LIBADD += $(top_builddir)/libsv/libsv.la
    libexample_la_DEPENDENCIES += $(top_builddir)/libsv/libsv.la

    $(top_builddir)/libsv/libsv.la:
    <tab>cd $(top_builddir)/libsv && $(MAKE) libsv.la

and add a configuration file `sv_config.h` in the include path which
defines HAVE_STDLIB_H etc. as needed by sv.h and sv.c

Optionally you might want in this file to redefine the exposed API
symbols with lines like:

    #define sv_new example_sv_new
    #define sv_free example_sv_free
    #define sv_set_option example_sv_set_option
    #define sv_get_line example_sv_get_line
    #define sv_get_header example_sv_get_header
    #define sv_parse_chunk example_sv_parse_chunk
    #define sv_write_fields example_sv_write_fields

You can see this demonstrated inside [Rasqal](https://github.com/dajobe/rasqal)

Example
-------

See `example.c` for API use.

Dave Beckett
http://www.dajobe.org/
