# Todos #

## High Priority (Core Functionality Gaps) ##

* Handle Nulls (missing values): allow at least ,, and ,"", and ,\N, for nulls in CSV (numpy `missing_values`) - Essential for real-world data with missing values
  * **Highest Priority**: Null values are everywhere - almost every real CSV file has missing data
  * **Current parsing likely fails**: Library probably treats `,,` as two empty strings rather than recognizing them as null/missing values
  * **Foundation for other features**: Many other parsing features depend on proper null handling
  * **Immediate practical impact**: This would fix parsing issues users encounter right away
  * **Three common null representations**:
    * `,,` - Empty fields between delimiters
    * `,""` - Empty quoted fields  
    * `,\N,` - Explicit null marker (common in some systems)
  * **numpy `missing_values` reference**: NumPy's CSV parser allows configuring what strings represent missing values (e.g., `missing_values=['', 'NA', 'NULL', '\\N']`) - this library should provide similar configurability
* Unicode encodings
  * [UTF-16](https://en.wikipedia.org/wiki/UTF-16) - Critical for international datasets
  * [The Absolute Minimum Everyone Working With Data Absolutely, Positively Must Know About File Types, Encoding, Delimiters and Data types (No Excuses!)](https://theonemanitdepartment.wordpress.com/2014/12/15/the-absolute-minimum-everyone-working-with-data-absolutely-positively-must-know-about-file-types-encoding-delimiters-and-data-types-no-excuses/)
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
