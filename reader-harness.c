#include "reader.h"
#include <klee/klee.h>
#include <assert.h>
#include <string.h>
/* Drive the extracted reader with a batch whose structure is concrete and whose
   amount and control-trailer low digits are symbolic, so the paths stay bounded.
   NREC selects the record count. */
#ifndef NREC
#define NREC 1
#endif
int main(void) {
  unsigned char buf[11 + NREC * 68 + 18];
  klee_make_symbolic(buf, sizeof buf, "buf");
  size_t p = 0; memcpy(buf, "H,PAYSLATE\n", 11); p = 11;
  for (int r = 0; r < NREC; r++) {
    buf[p] = 'D'; memset(buf + p + 1, 'A', 40);
    memcpy(buf + p + 41, "123-456", 7); memcpy(buf + p + 48, "000000001", 9);
    memset(buf + p + 57, '0', 8); buf[p + 67] = '\n'; p += 68;
  }
  buf[p] = 'T'; memset(buf + p + 1, '0', 5);
  memset(buf + p + 7, '0', 8); buf[p + 17] = '\n'; p += 18;
  struct record recs[NREC]; size_t n = 0; long long total = 0;
  int rc = read_batch(buf, p, recs, NREC, &n, &total);
  if (rc == 0) { long long s = 0; for (size_t i = 0; i < n; i++) s += recs[i].cents;
                 klee_assert((int) n == NREC && s == total); }
  return rc;
}
