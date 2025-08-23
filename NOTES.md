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
* Enable compiler warnings by default (`-Wall -Wextra -Wformat=2 -Wshadow -Wstrict-prototypes`) and treat warnings as errors in CI
* Fuzzing (operational): Short smoke fuzz job in CI (optional) and regularly merge seeds (e.g., csv-spectrum)
* Enforce strict quoted-cell validation when `SV_FLAGS_BAD_DATA_ERROR` is set (quote must be followed by separator/EOL)
* Run static analysis regularly (clang `--analyze`)

## Future (Nice-to-have) ##

* Convert fields to lower or upper case
* Set headers as parameter (ruby `headers` with array): implies no header line
* Skip lines regex (ruby `skip_lines` example is for comments '^#')
* Flag for empty line decision: an EOF, single empty field or no field?
* Allow ';' seps like ',' but for regions where , is in numbers (Excel in NO)
* ASCII delimited separators ASCII 28-31: 31 field sep, 30 record sep
* Fuzzing: Consider OSS-Fuzz integration for continuous large-scale fuzzing

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

## Developer Notes (2025-08-23)

- Build/test status: Clean build, all tests pass locally via GNUMakefile and svtest.
- Recommendations:
  - Enable compiler warnings by default (-Wall -Wextra -Wformat=2 -Wshadow -Wstrict-prototypes).
  - Implement BOM/encoding handling (at least UTF-8 BOM detection/stripping) or document current behavior explicitly.
  - Broaden allowed separators beyond ',' and '\t' (e.g., ';'), or document limitation.
  - Writer NULL semantics: consider serializing NULL as empty during output, or make it configurable.
  - Run static analysis regularly (clang --analyze).

## Fuzzing status

- Implemented: libFuzzer harnesses, Makefile targets, seed corpora, and dictionaries.
- See README “Developer: Fuzzing” for usage.
- Remaining items tracked in Todos:
  - Medium: short smoke fuzz in CI; regularly expand corpora (e.g., csv-spectrum) and merge seeds; enforce stricter quoted-cell checks under BAD_DATA_ERROR; run static analysis.
  - Future: consider OSS-Fuzz integration.
