# What KLEE found on the batch reader

All counts are from the `klee/klee:v3.2` image (amd64, emulated on Apple silicon),
measured on the code in this repository. No times are reported; only completed-path
and generated-test counts matter here.

## Path counts

| target | 1 record | 2 records | 3 records | shape |
| --- | --- | --- | --- | --- |
| toy reader (`toy.c`, one byte per amount) | 5 | 6 | 7 | linear, `N + 4` |
| extracted reader (`reader.c` + `reader-harness.c`) | 151 | 466 | 1411 | grows about ×3 per record |
| whole converter (`converter/aba-convert.c`, `--sym-files 1 20`) | 1102 | — | — | explodes at a 20-byte file |

The extracted-reader runs end with no error cases: the reconciliation assertion --
an accepted batch's record count and total equal what the control trailer declares
-- never fails, at one, two or three records.

## Reading the numbers

The toy reader stays small and linear because each amount is a single byte: per
record KLEE forks on the range check, and one branch carries on.

The whole converter, handed an unconstrained symbolic input file, is hopeless even
at twenty bytes -- every byte of header, record and trailer structure is another
fork, and the count runs away before it reaches the interesting logic.

Between the two sits the extracted reader. Pulling the parse-and-reconcile logic
out of the converter into a buffer function (`reader.c`, factored here from the
converter's single-file reader) lets KLEE call it with the structure fixed, so it
finishes at one, two and three records and exercises exactly the parse-and-reconcile
path worth checking. But the count still grows steeply -- about three times per
record, 151, 466, 1411. That is far from the toy's `N + 4`, and the reason is not
obvious from the counts alone; it is worth running down, because a reader whose
paths multiply with the record count is telling us something about how it reads a
record.

## What to do with this

Run KLEE on the extracted reader, not on the whole program: fix the structure, make
the amount and trailer digits symbolic, and it is bounded and solvable. Splitting
the reader out this way is worth doing in the converter itself. And the ×3 growth in
the extracted reader is the next thing to chase.
