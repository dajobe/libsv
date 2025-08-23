# libsv - Agent Guidelines

## Build Commands

- `make -f GNUMakefile` - Build library and executables
- `make check` - Run all tests
- `./svtest` - Run single test executable
- `make clean` - Clean build artifacts
- `make analyze` - Run clang static analyzer (maintainer mode)

## Architecture

- C library for CSV/TSV parsing and writing
- Core library: `libsv.a` built from `sv.c`, `option.c`, `write.c`, `read.c`
- Public API: `sv.h` with internal details in `sv_internal.h`
- Examples: `example.c`, `sv2c.c`, `gen.c`
- Test suite: `svtest.c` with test data files (test1.csv, test1.tsv, zero.tsv, one.tsv)

## Code Style

- ANSI C with 2-space indentation (`c-basic-offset: 2`)
- Use callback-based architecture for data processing
- Function naming: `sv_*` prefix for public API
- Type naming: `sv_*_t` suffix for enums/typedefs
- Error handling via `sv_status_t` enum return values
- Memory management: caller responsible for cleanup
- Include guards: conditional compilation with `#ifdef SV_CONFIG`
- Headers: system includes first, then `<sv.h>`, then internal headers
