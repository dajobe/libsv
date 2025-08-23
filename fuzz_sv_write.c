/*
 * fuzz_sv_write.c - libFuzzer harness for libsv writer
 *
 * Purpose:
 *  - Exercise the CSV writer with diverse field arrays, including NULLs
 *    and special characters, to catch crashes and sanitizer issues.
 *
 * Notes:
 *  - Current writer semantics: a NULL field pointer stops writing the row.
 *    This harness intentionally includes NULLs to exercise that path.
 *  - Consider changing writer behavior to serialize NULL as empty if desired.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sv.h>

int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (!data) return 0;

  sv* t = sv_new(NULL, NULL, NULL, ',');
  if (!t) return 0;

  enum { N = 8 };
  char* fields[N] = {0};
  size_t widths[N] = {0};

  /* Build up to N fields from input; include some NULLs deliberately */
  size_t i = 0, off = 0;
  while (i < N && off < size) {
    size_t len = (size - off > 16) ? (data[off] % 16) : (size - off);
    if (off + len > size) len = size - off;
    if ((data[off] & 3) == 0) {
      fields[i] = NULL; widths[i] = 0; /* exercise NULL behavior */
      off += (len ? len : 1);
    } else {
      fields[i] = (char*)malloc(len + 1);
      if (!fields[i]) break;
      memcpy(fields[i], data + off, len);
      fields[i][len] = '\0';
      widths[i] = len;
      off += len;
    }
    i++;
  }

  /* Write to /dev/null; on non-Unix systems, this may need a tweak */
  FILE* fh = fopen("/dev/null", "w");
  if (fh) {
    (void)sv_write_fields(t, fh, fields, widths, i);
    fclose(fh);
  }

  for (size_t j = 0; j < i; j++) free(fields[j]);
  sv_free(t);
  return 0;
}
