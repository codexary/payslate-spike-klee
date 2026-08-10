English | [简体中文](README.zh-CN.md)

# payslate-spike-klee

A short spike: learn how KLEE explores a batch reader, then measure how the path
count grows for a stripped-down toy reader, for the real converter's extracted
reader, and for the whole converter on unconstrained input. The point is to
decide where symbolic execution is worth running.

`toy.c` is a minimal reader -- a marker byte, a few one-byte amounts, a declared
count and total -- small enough to read every path KLEE reports.

```sh
sh run.sh 1      # run KLEE over the toy reader at one record, show a solved input
```

Everything runs in the official KLEE image (`klee/klee:v3.2`), which is amd64
only, so it runs under emulation on Apple-silicon machines. No times are reported;
the numbers that matter are path and test counts.

The path-count comparison and the conclusions are in [FINDINGS.md](FINDINGS.md).

## About this repository

Elsworne, Payslate and the team are fictional.

The code and documents in this repository were written with the help of AI tools; every recorded output comes from actually running them.
