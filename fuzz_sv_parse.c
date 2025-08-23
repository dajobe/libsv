/*
 * fuzz_sv_parse.c - libFuzzer harness for libsv parser
 *
 * Purpose:
 *  - Exercise the CSV/TSV parser with mutated inputs to discover crashes,
 *    out-of-memory behaviors, and undefined behavior.
 *  - Derives a few options (separator, whitespace stripping, null handling)
 *    from the first bytes of input, then feeds the rest as data in chunks.
 *
 * Notes:
 *  - This harness uses no-op callbacks; it only validates that parsing
 *    completes without crashing or triggering sanitizers.
 *  - Build with Clang + libFuzzer and ASan/UBSan (see GNUMakefile targets).
 */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sv.h>

/* No-op callbacks: accept all parsed rows/headers */
static sv_status_t noop_header(sv* t, void* u, char** f, size_t* w, size_t c){
  (void)t; (void)u; (void)f; (void)w; (void)c; return SV_STATUS_OK;
}
static sv_status_t noop_data(sv* t, void* u, char** f, size_t* w, size_t c){
  (void)t; (void)u; (void)f; (void)w; (void)c; return SV_STATUS_OK;
}

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (!data || size == 0) return 0;

  /* Derive a separator from first byte; allow ',', '\t', or ';' */
  char sep = (data[0] % 3 == 0) ? ',' : (data[0] % 3 == 1) ? '\t' : ';';
  sv* t = sv_new(NULL, noop_header, noop_data, sep);
  if (!t) return 0;

  /* Option toggles derived from next bytes when present */
  if (size > 1)
    sv_set_option(t, SV_OPTION_STRIP_WHITESPACE, (long)(data[1] & 1));
  if (size > 2)
    sv_set_option(t, SV_OPTION_NULL_HANDLING, (long)(data[2] & 1));

  /* Feed data in small chunks to exercise chunked parsing paths */
  size_t header = size > 4 ? 4 : size;
  const uint8_t* p = data + header;
  size_t remain = size - header;
  size_t chunk_seed = (size > 3 ? (size_t)data[3] : 17);
  while (remain > 0) {
    size_t max_chunk = 32;
    size_t chunk = (remain > max_chunk) ? (chunk_seed % max_chunk) + 1 : remain;
    (void)sv_parse_chunk(t, (char*)p, chunk);
    p += chunk;
    remain -= chunk;
  }
  (void)sv_parse_chunk(t, NULL, 0);

  sv_free(t);
  return 0;
}
