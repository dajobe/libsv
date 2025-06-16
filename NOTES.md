# Todos #

* Unicode encodings
  * [UTF-16](https://en.wikipedia.org/wiki/UTF-16)
  * [The Absolute Minimum Everyone Working With Data Absolutely, Positively Must Know About File Types, Encoding, Delimiters and Data types (No Excuses!)](https://theonemanitdepartment.wordpress.com/2014/12/15/the-absolute-minimum-everyone-working-with-data-absolutely-positively-must-know-about-file-types-encoding-delimiters-and-data-types-no-excuses/)
  * [The Absolute Minimum Every Software Developer Absolutely, Positively Must Know About Unicode and Character Sets (No Excuses!)](https://www.joelonsoftware.com/articles/Unicode.html)
* Handle Nulls (missing values): allow at least ,, and ,"", and ,\N,
  for nulls in CSV  (numpy `missing_values`)
* skip/select initial columns (numpy `usecols`; `skip columns` TDM)
* convert fields to lower or upper case
* warn on invalid number of cells in row compared to #headers
* field size limit (ruby `field_size_limit`)
* set headers as parameter (ruby `headers` with array): implies no
  header line
* skip lines regex (ruby `skip_lines` example is for comments '^#')
* flag for empty line decision: an EOF, single empty field or no field?
* allow ';' seps like ',' but for regions where , is in numbers (Excel in NO)
* ASCII delimited separators ASCII 28-31: 31 field sep, 30 record sep
* Skip blank rows - rows in which all cells are empty / 0-length  (`skipBlankRows` TDM)
* Original row numbers without skips and headers (`source numbers` of a row in TDM)

# Specifications #

* [Model for Tabular Data and Metadata on the Web][1][8]
from the W3C [CSV on the Web Working Group][2]
* [CSVW Namespace Vocabulary Terms][9]
* [ASCII delimited text][3]
* [Delimiter separated values][4]
* [Comma-separated values][5]
* [Data Protocols Tabular data package][6]
* [CSV dialect description format CSVDDF][7]
* 

# Articles and discussions #

* https://news.ycombinator.com/item?id=7795451
* https://news.ycombinator.com/item?id=7796268
* https://tburette.github.io/blog/2014/05/25/so-you-want-to-write-your-own-CSV-code/
* https://ronaldduncan.wordpress.com/2009/10/31/text-file-formats-ascii-delimited-text-not-csv-or-tab-delimited-text/

# Other implementations #

* C:
  [Miller](https://johnkerl.org/miller/doc/index.html)
  https://github.com/johnkerl/miller
* Python core:
  https://github.com/python/cpython/blob/master/Modules/_csv.c
  https://github.com/python/cpython/blob/master/Lib/csv.py
  https://github.com/python/cpython/blob/master/Lib/test/test_csv.py
* Python numpy: https://docs.scipy.org/doc/numpy/user/basics.io.genfromtxt.html
  https://github.com/numpy/numpy/blob/master/numpy/lib/npyio.py#L1257
* Ruby: https://github.com/ruby/ruby/blob/trunk/lib/csv.rb
* Perl: https://metacpan.org/pod/Text::CSV_XS
  https://github.com/Tux/Text-CSV_XS
  https://github.com/Tux/Text-CSV_XS/blob/master/CSV_XS.xs
  https://github.com/benbernard/RecordStream
* Javascript: https://github.com/mholt/PapaParse.git
  https://github.com/knrz/CSV.js.git
  https://github.com/d3/d3-dsv
  https://github.com/mafintosh/csv-parser
  https://github.com/koles/ya-csv.git

# Test suites #

* https://github.com/maxogden/csv-spectrum

# Large datasets #

* https://storage.googleapis.com/books/ngrams/books/datasetsv2.html


[1]: https://www.w3.org/TR/tabular-data-model/
[2]: https://www.w3.org/2013/csvw/wiki/Main_Page
[3]: https://en.wikipedia.org/wiki/Delimiter#ASCII_delimited_text
[4]: https://en.wikipedia.org/wiki/Delimiter-separated_values
[5]: https://en.wikipedia.org/wiki/Comma-separated_values
[6]: https://dataprotocols.org/tabular-data-package/#csv-files
[7]: https://dataprotocols.org/csv-dialect/
[8]: https://www.w3.org/TR/tabular-metadata/
[9]: https://www.w3.org/ns/csvw
