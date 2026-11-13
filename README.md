# Multi-threading: where it helps and where it doesn't

I'm learning multithreading, so I benchmarked matrix multiplication (n = 1000) with two algorithms, each run single-threaded and with two threads (pthreads).

## Build and run

```bash
gcc mm.c -o mm -O3 -march=native -pthread
./mm
1000        # matrix size n, read from stdin
```

## Results

Times are in seconds, n = 1000.

| Version | Single thread | Two threads | Speedup |
|---|---|---|---|
| Naive MM | 0.869 | 0.401 | ~2.2x |
| Tiled MM (B = 32) | 0.188 | 0.151 | ~1.25x |

## Key finding: pointer access slowed down my threaded code

My first threaded tile version was far slower than the single-threaded one (~0.9s vs 0.19s), even though it did the same work. There were two causes, both about what the compiler can't assume.

### 1. Struct fields and the global `n` get reloaded from memory

In the original loop, everything was read through `args->` and the global `n`:

```c
args->out[i*n+j] += r * args->m[k*n+j];
```

The compiler can't rule out that the store to `out[...]` changes `n`, `args->m` or `args->e`, so it reloads them after every store instead of keeping them in registers.

**Fix:** copy them into local variables once at the top of the function.

```c
const int N = n, B = a->B, e = a->e;
```

### 2. Pointer aliasing blocks vectorization

`out` and `m` are both plain `int*`, so the compiler can't prove they point to different memory. Suppose `out[j]` were the same memory as `m[j+1]`:

- iteration `j` writes `m[j+1]`
- iteration `j+1` reads `m[j+1]` and sees the new value

Each iteration would then depend on the previous one and must run in order. SIMD loads 8 values of `m` at once, which is only correct if earlier stores can't change later loads. Without that guarantee, the compiler emits scalar code, or adds a runtime overlap check with a slow fallback path.

**Fix:** declare the pointers `restrict`.

```c
const int* restrict m = a->m;
int* restrict out = a->out;
```

This is a promise to the compiler, not a check. It's true here because `M`, `m` and `out` come from three separate `malloc` calls, so they never overlap. If the promise were false, the result would be wrong with no warning.

Separately, the two threads write disjoint rows of `out`. That is unrelated to `restrict`. It's what prevents a data race between the threads.

### Final thread function

```c
void* thread_tile_mm(void* arg) {
    struct mm *a = (struct mm*)arg;
    const int N = n, B = a->B, e = a->e;
    const int* restrict M = a->M;
    const int* restrict m = a->m;
    int* restrict out = a->out;

    for (int si = a->s; si < e; si += B)
        for (int sj = 0; sj < N; sj += B)
            for (int sk = 0; sk < N; sk += B)
                for (int i = si; i < si + B && i < e; i++)
                    for (int k = sk; k < sk + B && k < N; k++) {
                        int r = M[i*N + k];
                        for (int j = sj; j < sj + B && j < N; j++)
                            out[i*N + j] += r * m[k*N + j];
                    }
    return NULL;
}
```

## Takeaways

- The naive version scaled well because it is compute-bound.
- The tiled version only got ~1.25x from two threads. It is already cache-efficient and fast (0.19s), so thread startup cost and shared memory bandwidth become a larger share of the runtime.
- In hot loops, read from structs and globals once into locals, and use `restrict` when you know the pointers don't overlap.

## Next steps

- Time with a larger `n` (e.g. 2000), since single runs at n = 1000 are noisy and short.
- Repeat each run several times and average.
- Try more threads and see where the tiled version stops scaling.
- Check vectorization with `-fopt-info-vec-missed`.
