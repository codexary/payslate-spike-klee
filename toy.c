#include <klee/klee.h>
#include <assert.h>

/* A toy batch, stripped down to show how KLEE explores a reader: a one-byte
   marker, NREC one-byte amounts, a declared count and a declared total. Each
   amount is a single byte here, not the ten ASCII digits aba-convert reads --
   a simplification that keeps the paths few enough to read one by one. */
#ifndef NREC
#define NREC 1
#endif

static int toy_read(const unsigned char *buf, int n, int *total_out) {
  if (buf[0] != 'B') return 1;             /* marker */
  if (buf[1] != n) return 2;               /* declared count */
  int sum = 0;
  for (int i = 0; i < n; i++) {
    unsigned char a = buf[2 + i];
    if (a > 100) return 3;                 /* per-amount range */
    sum += a;
  }
  if (buf[2 + n] != sum) return 4;         /* reconcile declared total */
  *total_out = sum;
  return 0;
}

int main(void) {
  unsigned char buf[3 + NREC];
  klee_make_symbolic(buf, sizeof buf, "buf");
  int total = 0;
  int rc = toy_read(buf, NREC, &total);
  if (rc == 0) klee_assert(total >= 0 && total <= 100 * NREC);
  return rc;
}
