#!/bin/sh
# Run KLEE over the toy reader and show one solved input with ktest-tool. Needs
# Docker; the KLEE image is amd64 only and runs under emulation on Apple silicon.
# NREC selects the record count (default 1). No times are reported.
set -eu
cd "$(dirname "$0")"
img=klee/klee:v3.2
nrec="${1:-1}"
rm -rf klee-out toy.bc
docker run --rm --platform linux/amd64 -v "$PWD:/src" -w /src "$img" sh -c "
  clang -emit-llvm -c -g -O0 -Xclang -disable-O0-optnone -DNREC=${nrec} toy.c -o toy.bc
  klee --output-dir=klee-out toy.bc
  echo '--- one solved input ---'
  ktest-tool klee-out/test000001.ktest
"
