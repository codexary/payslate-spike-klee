/* Payslate desktop ABA batch converter, carried over from the desktop product. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_RECORDS 999999   /* the control trailer's 6-digit count field */

struct record {
  char account_name[25];
  char bsb[8];
  char account[10];
  long long cents;
};

static void field(char *out, const char *line, int start, int width) {
  memcpy(out, line + start, width);
  out[width] = '\0';
  for (int i = width - 1; i >= 0 && out[i] == ' '; i--) out[i] = '\0';
}

/* Parse one detail line into a record. Returns 0 on success, 2 with a message. */
static int parse_detail(const char *line, struct record *r) {
  char source_name[41];
  char amount[11];
  if (line[0] != 'D' || strlen(line) < 67) {
    fprintf(stderr, "invalid detail\n");
    return 2;
  }
  field(source_name, line, 1, 40);
  /* Bound the copy to the 24-character output field (plus a NUL). */
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

int main(int argc, char **argv) {
  if (argc != 2) {
    fprintf(stderr, "usage: aba-convert input.txt\n");
    return 2;
  }
  FILE *in = fopen(argv[1], "r");
  if (in == NULL) {
    fprintf(stderr, "usage: aba-convert input.txt\n");
    return 2;
  }
  char line[256];
  if (fgets(line, sizeof line, in) == NULL || strncmp(line, "H,PAYSLATE", 10) != 0) {
    fprintf(stderr, "invalid header\n");
    fclose(in);
    return 2;
  }
  /* Read the whole input once, keeping the parsed records in memory. Write
     nothing until the control trailer reconciles, so a file that changes under
     us (a copy still in flight) cannot pass validation and then be re-read. */
  struct record *recs = NULL;
  size_t n = 0, cap = 0;
  long long total = 0;
  int trailer_seen = 0;
  int declared_count = 0;
  long long declared_total = 0;
  int rc = 2;
  while (fgets(line, sizeof line, in) != NULL) {
    if (trailer_seen) {
      fprintf(stderr, "data after control trailer\n");
      goto done;
    }
    if (line[0] == 'T') {
      /* Control trailer: T + 6-digit count + 10-digit total in cents, then only
         an optional line ending. */
      size_t len = strlen(line);
      if (len < 17) { fprintf(stderr, "invalid control trailer\n"); goto done; }
      int bad = 0;
      for (int i = 1; i <= 16; i++)
        if (line[i] < '0' || line[i] > '9') bad = 1;
      const char *rest = line + 17;
      if (!(rest[0] == '\0'
            || (rest[0] == '\n' && rest[1] == '\0')
            || (rest[0] == '\r' && rest[1] == '\n' && rest[2] == '\0')))
        bad = 1;
      if (bad) { fprintf(stderr, "invalid control trailer\n"); goto done; }
      char dc[7], dt[11];
      field(dc, line, 1, 6);
      field(dt, line, 7, 10);
      declared_count = (int) strtol(dc, NULL, 10);
      declared_total = strtoll(dt, NULL, 10);
      trailer_seen = 1;
      continue;
    }
    struct record r;
    if (parse_detail(line, &r) != 0) goto done;
    total += r.cents;
    if (total > 9999999999LL) { fprintf(stderr, "batch total out of range\n"); goto done; }
    if (n == cap) {
      if (n >= MAX_RECORDS) { fprintf(stderr, "too many records\n"); goto done; }
      size_t ncap = cap ? cap * 2 : 16;
      if (ncap > MAX_RECORDS) ncap = MAX_RECORDS;
      struct record *nr = realloc(recs, ncap * sizeof *recs);
      if (nr == NULL) { fprintf(stderr, "out of memory\n"); goto done; }
      recs = nr;
      cap = ncap;
    }
    recs[n++] = r;
  }
  if (ferror(in)) {
    fprintf(stderr, "read error\n");
    goto done;
  }
  if (!trailer_seen) {
    fprintf(stderr, "missing control trailer; re-export from Payslate 7\n");
    goto done;
  }
  if (n == 0) {
    fprintf(stderr, "empty batch; nothing to convert\n");
    goto done;
  }
  if (declared_count != (int) n || declared_total != total) {
    fprintf(stderr, "control trailer does not reconcile\n");
    goto done;
  }
  /* The batch reconciles: write the ABA output from the records held in memory. */
  puts("0                 PAYSLATE SYNTHETIC BATCH");
  for (size_t i = 0; i < n; i++)
    printf("1%-7s%-9s%-24s%010lld\n", recs[i].bsb, recs[i].account, recs[i].account_name, recs[i].cents);
  printf("7%010lld%06d\n", total, (int) n);
  rc = 0;
done:
  free(recs);
  fclose(in);
  return rc;
}
