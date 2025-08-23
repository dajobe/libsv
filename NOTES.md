# Todos #

## High Priority (Core Functionality Gaps) ##

* Unicode encodings
  * [UTF-16](https://en.wikipedia.org/wiki/UTF-16) - Critical for international datasets
  * [The Absolute Minimum Everyone Working With Data Absolutely, Positively Must Know About File Types, Encoding, Delimiters and Data Types (No Excuses!)](https://theonemanitdepartment.wordpress.com/2014/12/15/the-absolute-minimum-everyone-working-with-data-absolutely-positively-must-know-about-file-types-encoding-delimiters-and-data-types-no-excuses/)
  * [The Absolute Minimum Every Software Developer Absolutely, Positively Must Know About Unicode and Character Sets (No Excuses!)](http://www.joelonsoftware.com/articles/Unicode.html)
* Field size limit (ruby `field_size_limit`) - Prevents buffer overflows and memory issues with large fields
  * **Security & Stability**: Prevents denial-of-service attacks from malicious CSV files with extremely large fields
  * **Current gap**: Library has no hard limit on individual field sizes - could consume unlimited memory
  * **Risk**: A single field with gigabytes of data could cause memory exhaustion and crashes
  * **Existing protections**: Code already has buffer growth logic and integer overflow checks, but no field size caps
  * **Implementation**: Would set maximum field size (e.g., 1MB, 10MB) and either truncate, skip, or error on oversized fields

## Medium Priority (Data Quality & Usability) ##

* Skip blank rows - rows in which all cells are empty / 0-length (`skipBlankRows` TDM) - Common need when processing real data
* Warn on invalid number of cells in row compared to #headers - Helps catch malformed data early
* Skip/select initial columns (numpy `usecols`; `skip columns` TDM) - Common requirement for data preprocessing
* Original row numbers without skips and headers (`source numbers` of a row in TDM) - Useful for debugging and data lineage

## Future (Nice-to-have) ##

* Convert fields to lower or upper case
* Set headers as parameter (ruby `headers` with array): implies no header line
* Skip lines regex (ruby `skip_lines` example is for comments '^#')
* Flag for empty line decision: an EOF, single empty field or no field?
* Allow ';' seps like ',' but for regions where , is in numbers (Excel in NO)
* ASCII delimited separators ASCII 28-31: 31 field sep, 30 record sep

## Code Quality Assessment ##

**Overall Grade: B+ (Good with room for improvement)**

**Strengths:**

* Excellent API design and documentation in headers
* Good memory management and error handling
* Clean separation of concerns between modules
* Recent null handling improvements show good design thinking

**Areas for Improvement:**

* **File sizes**: `read.c` (925 lines) and `svtest.c` (1321 lines) exceed recommended 200-300 line limits
* **Complexity**: Some parsing logic could be simplified and refactored
* **Organization**: Large files make maintenance and navigation difficult

**Code Complexity Metrics (lizard analysis):**

* **High Complexity Functions** (CCN > 15):
  * `sv_internal_parse_process_char` (CCN: 79, NLOC: 168) - Core parser state machine - **NOTE: This function is actually well-designed and readable despite high CCN. The high complexity reflects the breadth of parsing states handled, not confusing logic. Each switch case is independent and focused.**
  * `sv_set_option_vararg` (CCN: 47, NLOC: 147) - Option handling switch statement - **NOTE: This function is also well-designed despite high CCN. The complexity comes from handling many different option types in a clean switch statement, each case being focused and independent.**
  * `sv_parse_generate_row` (CCN: 26, NLOC: 88) - Row generation logic
  * `sv_write_field` (CCN: 21, NLOC: 39) - Field escaping logic
* **Large Functions** (NLOC > 100):
  * `main` functions in example.c (108 lines). This is ok to leave alone as it is an example.
* `main` functions in svtest.c (90 lines).

**Recommendations:**

* Split large files into focused, smaller modules
* Extract common patterns into utility functions
* Consider breaking parser into state machine, core parsing, and utilities
* Refactor high-complexity functions like `sv_internal_parse_process_char`
* Split option handling by category (parsing, output, null handling)

## Specifications ##

* [Model for Tabular Data and Metadata on the Web][1][8]
from the W3C [CSV on the Web Working Group][2]
* [CSVW Namespace Vocabulary Terms][9]
* [ASCII delimited text][3]
* [Delimiter separated values][4]
* [Comma-separated values][5]
* [Data Protocols Tabular data package][6]
* [CSV dialect description format CSVDDF][7]
*

## Articles and discussions ##

* <https://news.ycombinator.com/item?id=7795451>
* <https://news.ycombinator.com/item?id=7796268>
* <http://tburette.github.io/blog/2014/05/25/so-you-want-to-write-your-own-CSV-code/>
* <https://ronaldduncan.wordpress.com/2009/10/31/text-file-formats-ascii-delimited-text-not-csv-or-tab-delimited-text/>

## Other implementations ##

* C:
  [Miller](http://johnkerl.org/miller/doc/index.html)
  <https://github.com/johnkerl/miller>
* Python core:
  <https://github.com/python/cpython/blob/master/Modules/_csv.c>
  <https://github.com/python/cpython/blob/master/Lib/csv.py>
  <https://github.com/python/cpython/blob/master/Lib/test/test_csv.py>
* Python numpy: <http://docs.scipy.org/doc/numpy/user/basics.io.genfromtxt.html>
  <https://github.com/numpy/numpy/blob/master/numpy/lib/npyio.py#L1257>
* Ruby: <https://github.com/ruby/ruby/blob/trunk/lib/csv.rb>
* Perl: <https://metacpan.org/pod/Text::CSV_XS>
  <https://github.com/Tux/Text-CSV_XS>
  <https://github.com/Tux/Text-CSV_XS/blob/master/CSV_XS.xs>
  <https://github.com/benbernard/RecordStream>
* Javascript: <https://github.com/mholt/PapaParse.git>
  <https://github.com/knrz/CSV.js.git>
  <https://github.com/d3/d3-dsv>
  <https://github.com/mafintosh/csv-parser>
  <https://github.com/koles/ya-csv.git>

## Test suites ##

* <https://github.com/maxogden/csv-spectrum>

## Large datasets ##

* <http://storage.googleapis.com/books/ngrams/books/datasetsv2.html>

[1]: http://www.w3.org/TR/tabular-data-model/
[2]: http://www.w3.org/2013/csvw/wiki/Main_Page
[3]: https://en.wikipedia.org/wiki/Delimiter#ASCII_delimited_text
[4]: https://en.wikipedia.org/wiki/Delimiter-separated_values
[5]: https://en.wikipedia.org/wiki/Comma-separated_values
[6]: http://dataprotocols.org/tabular-data-package/#csv-files
[7]: http://dataprotocols.org/csv-dialect/
[8]: http://www.w3.org/TR/tabular-metadata/
[9]: http://www.w3.org/ns/csvw

## Recent Evaluation (2025-08-23)

* Build/test status: Clean build, all tests pass locally using GNUMakefile.
* Recommendations:
  * Enable compiler warnings by default (-Wall -Wextra -Wformat=2 -Wshadow -Wstrict-prototypes) and wire CI to treat warnings as errors.
  * Implement BOM/encoding handling (at least UTF-8 BOM detection/stripping) or document current behavior explicitly.
  * Broaden allowed separators beyond ',' and '\t' (e.g., ';'), or document limitation.
  * Writer NULL semantics: treat NULL fields as empty during output rather than truncating the row, or make it configurable.
  * Add fuzzing with libFuzzer + sanitizers (ASan/UBSan) for the parser and writer.
  * Run static analysis regularly (clang --analyze) in CI.

## Fuzzing Plan (libFuzzer)

libFuzzer is a coverage-guided fuzzing engine built into Clang. It repeatedly mutates inputs to maximize code coverage and uncover crashes, OOMs, and undefined behavior. It works best when combined with AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).

Targets

1) Parser target (fuzz_sv_parse)
   * Entry: LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
   * Behavior:
     * Create an sv instance with randomized options derived from the input prefix (e.g., first few bytes encode sep, quote, escape, flags). Fall back to safe defaults if invalid.
     * Feed the remaining bytes to sv_parse_chunk in both whole and chunked modes (e.g., random chunk sizes) and finalize with sv_parse_chunk(NULL, 0).
     * Use no-op but robust callbacks that tolerate NULL fields and arbitrary widths (validate invariants but avoid heavy allocation).
   * Invariants to assert: no crashes, no memory leaks (ASan), no UB (UBSan). Optionally, enforce that the state machine never returns an error unless input is malformed and strict-mode is on.

2) Writer target (fuzz_sv_write)
   * Generate small arrays of fields/widths from input. Include cases with NULL pointers, embedded separators, quotes, escapes, CR/LF, long fields.
   * Call sv_write_fields to a tmpfile or open_memstream equivalent; verify it returns success or fails gracefully without crashing.

Seed corpus and dictionary

* Seed with existing small CSV/TSV examples and edge cases from the test suite (e.g., quotes, CRLF, empty fields, NA, NULL, \\N).
* Pull samples from csv-spectrum.
* Dictionary tokens: ',', '\t', ';', '"', '\n', '\r\n', '\\', 'NA', 'NULL', '\\N', "''".

Build integration

* Add a dedicated fuzz build using Clang and libFuzzer with sanitizers.
* Example GNUMakefile additions:

```make
FUZZ_CFLAGS = -O1 -g -fsanitize=fuzzer,address,undefined -fno-omit-frame-pointer
FUZZ_LDFLAGS = -fsanitize=fuzzer,address,undefined

fuzz_sv_parse: fuzz_sv_parse.o libsv.a
 $(CC) $(FUZZ_CFLAGS) -I. -o $@ $^ $(FUZZ_LDFLAGS)

fuzz_sv_write: fuzz_sv_write.o libsv.a
 $(CC) $(FUZZ_CFLAGS) -I. -o $@ $^ $(FUZZ_LDFLAGS)

# Generic rule
%.o: %.c
 $(CC) $(FUZZ_CFLAGS) -I. -c -o $@ $<
```

Harness skeletons

* fuzz_sv_parse.c:

```c
#include <stdint.h>
#include <stdlib.h>
#include <sv.h>

static sv_status_t noop_header(sv* t, void* u, char** f, size_t* w, size_t c){ return SV_STATUS_OK; }
static sv_status_t noop_data(sv* t, void* u, char** f, size_t* w, size_t c){ return SV_STATUS_OK; }

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (!data || size == 0) return 0;
  // Derive simple options from the first few bytes
  char sep = (data[0] % 3 == 0) ? ',' : (data[0] % 3 == 1) ? '\t' : ';';
  sv* t = sv_new(NULL, noop_header, noop_data, sep);
  if (!t) return 0;

  // Optional: randomize a few options
  // sv_set_option(t, SV_OPTION_STRIP_WHITESPACE, (long)(data[1] & 1));
  // sv_set_option(t, SV_OPTION_NULL_HANDLING, (long)(data[2] & 1));

  // Feed data in chunks
  const size_t header = 4 < size ? 4 : size;
  const uint8_t* p = data + header;
  size_t remain = size - header;
  while (remain > 0) {
    size_t chunk = (remain > 32) ? (data[3] % 32) + 1 : remain;
    (void)sv_parse_chunk(t, (char*)p, chunk);
    p += chunk;
    remain -= chunk;
  }
  (void)sv_parse_chunk(t, NULL, 0);
  sv_free(t);
  return 0;
}
```

* fuzz_sv_write.c:

```c
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sv.h>

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (!data) return 0;
  sv* t = sv_new(NULL, NULL, NULL, ',');
  if (!t) return 0;

  // Build up to N small fields from input, include some NULLs
  enum { N = 8 };
  char* fields[N] = {0};
  size_t widths[N] = {0};
  size_t i = 0, off = 0;
  while (i < N && off < size) {
    size_t len = (size - off > 16) ? (data[off] % 16) : (size - off);
    if (off + len > size) len = size - off;
    if ((data[off] & 3) == 0) {
      fields[i] = NULL; widths[i] = 0; // exercise NULL behavior
    } else {
      fields[i] = (char*)malloc(len + 1);
      if (!fields[i]) break;
      memcpy(fields[i], data + off, len);
      fields[i][len] = '\0';
      widths[i] = len;
    }
    off += len ? len : 1;
    i++;
  }

  FILE* fh = fopen("/dev/null", "w");
  if (fh) {
    (void)sv_write_fields(t, fh, fields, widths, i);
    fclose(fh);
  }

  for (size_t j = 0; j < i; j++) free(fields[j]);
  sv_free(t);
  return 0;
}
```

How to run locally

* Build: CC=clang make -f GNUMakefile fuzz_sv_parse fuzz_sv_write
* Run: ./fuzz_sv_parse -max_total_time=300 -timeout=10 corpus/parse
* Run: ./fuzz_sv_write -max_total_time=300 -timeout=10 corpus/write
* Use -jobs and -workers for parallel runs; add -dict=dict.txt for token guidance.

CI integration

* Add a GitHub Actions job that builds the fuzzers with Clang and runs each for ~5 minutes on push/PR.
* Artifacts: upload any crashers/minimized reproducers from the fuzz run.

OSS-Fuzz (optional)

* Create a minimal Dockerfile and build script; register the project with OSS-Fuzz for continuous large-scale fuzzing.

Triage and regression

* On crash: use -minimize_crash=1 to shrink inputs; add the minimized input to the seed corpus and/or convert into a unit test in svtest.c.
* Tag findings with CVE-style notes if they are memory-safety relevant; fix with tests before closing.

Milestones

[ ] Land harnesses + make targets, seed corpus, enable ASan/UBSan; 5 min CI fuzz smoke.
[ ] Expand corpora (csv-spectrum), add dictionary, fix any found issues; wire strict-mode quote checks.
[ ] Consider field size limits, separator expansion, BOM handling; pursue OSS-Fuzz.

Acceptance criteria

* Fuzzers run clean for 10k+ exec/s locally with no crashes for 10+ minutes.
* CI smoke fuzz completes within 5 minutes with zero findings.
* All discovered issues have tests and are closed.
