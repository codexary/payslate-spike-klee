#include "reader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static void field(char *out, const char *line, int start, int width) {
  memcpy(out, line + start, width);
  out[width] = '\0';
  for (int i = width - 1; i >= 0 && out[i] == ' '; i--) out[i] = '\0';
}

/* Copy the line starting at *pos (up to a newline, which is kept) into out,
   NUL-terminated and capped at cap-1 bytes, and advance *pos past it. Returns
   the length copied. */
static size_t next_line(const unsigned char *buf, size_t len, size_t *pos,
                        char *out, size_t cap) {
  size_t i = 0;
  while (*pos < len && i + 1 < cap) {
    char c = (char) buf[*pos];
    out[i++] = c;
    (*pos)++;
    if (c == '\n') break;
  }
  out[i] = '\0';
  return i;
}

static int parse_detail(const char *line, struct record *r) {
  char source_name[41];
  char amount[11];
  if (line[0] != 'D' || strlen(line) < 67) {
    fprintf(stderr, "invalid detail\n");
    return 2;
  }
  field(source_name, line, 1, 40);
  snprintf(r->account_name, sizeof r->account_name, "%.24s", source_name);
  field(r->bsb, line, 41, 7);
  field(r->account, line, 48, 9);
  field(amount, line, 57, 10);
  if (amount[0] == '\0') { fprintf(stderr, "invalid amount\n"); return 2; }
  for (int i = 0; amount[i] != '\0'; i++)
    if (amount[i] < '0' || amount[i] > '9') {
      fprintf(stderr, "invalid amount\n");
      return 2;
    }
  errno = 0;
  char *end;
  long long c = strtoll(amount, &end, 10);
  if (errno != 0 || *end != '\0' || c < 0 || c > 9999999999LL) {
    fprintf(stderr, "amount out of range\n");
    return 2;
  }
  r->cents = c;
  return 0;
}

int read_batch(const unsigned char *buf, size_t len, struct record *recs,
               size_t max, size_t *n_out, long long *total_out) {
  char line[256];
  size_t pos = 0;
  next_line(buf, len, &pos, line, sizeof line);
  if (strncmp(line, "H,PAYSLATE", 10) != 0) {
    fprintf(stderr, "invalid header\n");
    return 2;
  }
  size_t n = 0;
  long long total = 0;
  int trailer_seen = 0;
  int declared_count = 0;
  long long declared_total = 0;
  while (pos < len) {
    next_line(buf, len, &pos, line, sizeof line);
    if (trailer_seen) {
      fprintf(stderr, "data after control trailer\n");
      return 2;
    }
    if (line[0] == 'T') {
      size_t tlen = strlen(line);
      if (tlen < 17) { fprintf(stderr, "invalid control trailer\n"); return 2; }
      int bad = 0;
      for (int i = 1; i <= 16; i++)
        if (line[i] < '0' || line[i] > '9') bad = 1;
      const char *rest = line + 17;
      if (!(rest[0] == '\0'
            || (rest[0] == '\n' && rest[1] == '\0')
            || (rest[0] == '\r' && rest[1] == '\n' && rest[2] == '\0')))
        bad = 1;
      if (bad) { fprintf(stderr, "invalid control trailer\n"); return 2; }
      char dc[7], dt[11];
      field(dc, line, 1, 6);
      field(dt, line, 7, 10);
      declared_count = (int) strtol(dc, NULL, 10);
      declared_total = strtoll(dt, NULL, 10);
      trailer_seen = 1;
      continue;
    }
    struct record r;
    if (parse_detail(line, &r) != 0) return 2;
    total += r.cents;
    if (total > 9999999999LL) { fprintf(stderr, "batch total out of range\n"); return 2; }
    if (n >= max) { fprintf(stderr, "too many records\n"); return 2; }
    recs[n++] = r;
  }
  if (!trailer_seen) {
    fprintf(stderr, "missing control trailer; re-export from Payslate 7\n");
    return 2;
  }
  if (n == 0) {
    fprintf(stderr, "empty batch; nothing to convert\n");
    return 2;
  }
  if (declared_count != (int) n || declared_total != total) {
    fprintf(stderr, "control trailer does not reconcile\n");
    return 2;
  }
  *n_out = n;
  *total_out = total;
  return 0;
}

void write_batch(const struct record *recs, size_t n, long long total) {
  puts("0                 PAYSLATE SYNTHETIC BATCH");
  for (size_t i = 0; i < n; i++)
    printf("1%-7s%-9s%-24s%010lld\n", recs[i].bsb, recs[i].account, recs[i].account_name,
           recs[i].cents);
  printf("7%010lld%06d\n", total, (int) n);
}
