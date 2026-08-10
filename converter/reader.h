#ifndef CONVERT_H
#define CONVERT_H

#include <stddef.h>

#define MAX_RECORDS 999999 /* the control trailer's 6-digit count field */

struct record {
  char account_name[25];
  char bsb[8];
  char account[10];
  long long cents;
};

/* Parse and reconcile a batch held entirely in a buffer, with no I/O, so a
   symbolic engine can exercise it directly. Fills recs[0..*n_out) and *total_out
   and returns 0 when the header, record count, total and trailer all reconcile;
   otherwise prints a reason to stderr and returns 2. recs holds at most max
   records. */
int read_batch(const unsigned char *buf, size_t len, struct record *recs,
               size_t max, size_t *n_out, long long *total_out);

/* Write the ABA layout for a reconciled batch to standard output. */
void write_batch(const struct record *recs, size_t n, long long total);

#endif
