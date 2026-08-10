#!/bin/sh
# Count KLEE completed paths and generated tests for (1) the whole converter on an
# unconstrained 20-byte symbolic input file and (2) the extracted reader at 1, 2
# and 3 records. Needs Docker (amd64 image, emulated on Apple silicon). No times
# are reported. Usage: sh count-paths.sh
set -eu
cd "$(dirname "$0")"
img=klee/klee:v3.2
run() { docker run --rm --platform linux/amd64 -v "$PWD:/src" -w /src "$img" sh -c "$1"; }
echo "== whole converter, --sym-files 1 20 =="
run 'clang -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone converter/aba-convert.c -o whole.bc 2>/dev/null;
     klee --posix-runtime --libc=uclibc --output-dir=klee-out-whole whole.bc A --sym-files 1 20 2>&1 | grep -E "completed paths|generated tests"' || true
echo "== extracted reader at 1/2/3 records =="
for n in 1 2 3; do
  printf '  NREC=%s: ' "$n"
  run "clang -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -DNREC=$n -I converter reader-harness.c -o h.bc;
       clang -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -I converter converter/reader.c -o r.bc;
       llvm-link h.bc r.bc -o rc.bc;
       klee --output-dir=klee-out-reader-$n --libc=uclibc rc.bc 2>&1 | grep 'completed paths'" || true
done
