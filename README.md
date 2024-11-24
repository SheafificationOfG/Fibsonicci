# Fibsonicci

Source code for all of the implementations (and a bit more) in my video: [One second to compute the largest Fibonacci number I can](https://youtu.be/KzT9I1d-LlQ).

[![One second to compute the largest Fibonacci number I can](https://img.youtube.com/vi/KzT9I1d-LlQ/0.jpg)](https://youtu.be/KzT9I1d-LlQ)

The video is a hard prerequisite to coming here and making fun of my code.

# Usage

After cloning the repo, make sure to run

```bash
make init
```

to initialise the output folders before trying to build anything specific.

## Computing one Fibonacci number

To compute a specific Fibonacci number using one of the [given implementations](#algorithms), use

```bash
make bin/one_$(algo).O3.out
./bin/one_$(algo).O3.out
```

If you are only interested in the computation time, run

```bash
make clean-bin # or just ensure that ./bin/one_$(algo) doesn't exist
make FLAGS="-DPERF" one_$(algo)
```

### Full expansion

By default, the output will be in scientific notation (with an option to fully expand).
However, since Fibonacci numbers are encoded in a dyadic base, the expansion in decimal is achieved with a very lazily-implemented $`O(n^2)`$ double-dabble, so can be very slow if the result has a large number of digits.
If you really want to verify the correctness of the output, then run

```bash
make clean-bin # or just ensure that ./bin/one_$(algo) doesn't exist
make FLAGS="-DCHECK" one_$(algo)
```

Then, the outputs will be in hexadecimal, and fully-expanded.

## Generating runtime plots

> **Note.** The runtime generator (in particular, its attempt to find the maximum Fibonacci number computable by a given algorithm within 1 second) is *highly nonscientific* and inaccurate.
> I made no effort to ensure that the data from the video is perfectly replicable!

To generate the data for *all* algorithms, simply run

```bash
make all-data
```

# Algorithms

Large numbers are encoded as base-$`2^L`$ unsigned integers (using `vector`s), where $`L`$ depends on the algorithm of choice.
This project includes the following implementations:

| Algorithm | Source (`impl/`) | Runtime | Digit size ($`2^L`$) |
|:---------:|:----------------:|:-------:|:--------------------:|
| [Naive](#naive) | `naive.cpp` | $`\Omega(\exp(n))`$ | $`2^{64}`$ |
| ["Linear"](#linear) | `linear.cpp` | $`O(n^2)`$ | $`2^{64}`$ |
| [Simple matrix multiplication](#simple-matrix-multiplication) | `matmul_simple.cpp` | $`O(n^2)`$ | $`2^{32}`$ |
| [Fast exponentiation](#fast-exponentiation) | `matmul_fastexp` | $`O(n^2)`$ | $`2^{32}`$ |
| [Strassen matrix multiplication](#strassen-matrix-multiplication) | `matmul_strassen.cpp` | $`O(n^2)`$ | $`2^{32}`$ |
| [Karatsuba multiplication](#karatsuba-multiplication) | `matmul_karatsuba.cpp` | $`O(n^{1.585})`$ | $`2^{32}`$ |
| [DFT](#dft) | `matmul_dft.cpp` | $`O(n^2)`$[^1] | $`2^8`$ |
| [FFT](#fft) | `matmul_fft.cpp` | $`O(n\log n)`$[^1] | $`2^8`$ |
| [Binet formula](#binet-formula) | `field_ext.cpp` | $`O(n\log n)`$[^1] | $`2^8`$ |

[^1]: These algorithms eventually fail (due to exceeding floating-point precision) when `n` is sufficiently large (e.g., fails when `n >= 0x7f'ffff`).


## Naive

The "naïve" implementation is just the simple (non-memoised) recursive algorithm.

## Linear

The "linear" implementation is the direct non-recursive implementation.
Of course, the algorithm is not *actually* linear; the runtime is $`O(n^2)`$.

## Simple matrix multiplication

Naïve implementation based on the identity

```math
\begin{bmatrix}
F_n \\ F_{n+1}
\end{bmatrix}
=
\begin{bmatrix}
0 & 1 \\ 1 & 1
\end{bmatrix}^n
\begin{bmatrix}
0 \\ 1
\end{bmatrix}
```

## Fast exponentiation

This implementation improves on the [simpler variant](#simple-matrix-multiplication) above by using the $`O(\log n)`$ fast exponentiation algorithm.

## Strassen matrix multiplication

This implementation modifies the [fast exponentiation](#fast-exponentiation) algorithm to use Strassen's matrix multiplication algorithm, which reduces the number of integer multiplications down from $`8`$ to $`7`$.

This modification was not mentioned in the video, since it leads to minimal improvement over naïve matrix multiplication (it would be a different story if matrices were larger than $`2\times2`$).

## Karatsuba multiplication

This implementation enhances the [fast exponentiation](#fast-exponentiation) algorithm by replacing the naïve grade-school integer multiplication algorithm with Karatsuba's $`O(n^{\log_23})`$ algorithm.

Note, however, that my implementation doesn't lead to any noticeable results until $`n\gg0`$ (you can definitely feel it when $`n\geq2^{24}`$).

## DFT

This implementation takes the [fast exponentiation](#fast-exponentiation) algorithm and replaces its grade-school integer multiplication with integer multiplication based on the discrete Fourier transform.

The transform is over the field of complex numbers, which are represented with pairs of `double`-precision floating-point numbers, so the algorithm's correctness is limited by this.
(I was too lazy to implement custom-precision floats.)

## FFT

This improves the [DFT](#dft) algorithm with the Cooley-Tukey Fast Fourier Transform.
Of course, this suffers from the same precision limitation.

## Binet formula

Finally deviating from the matrix multiplication algorithms above, this algorithm is based on Binet's formula

```math
F_n = \frac{\varphi^n - (-\varphi)^{-n}}{\sqrt5}
```

(where $`\varphi`$ is the golden ratio).

More precisely, we compute the coefficient of $`\sqrt5`$ in the expansion of $`\varphi^n`$ in the ring of integers of $`\mathbb{Q}(\sqrt5)`$.
Note that this computation can really be done in $`\mathbb{N}[\sqrt5]`$---and it is.

Large integer multiplication is achieved with [FFTs](#fft), so suffers from the same precision limitation as the previous two algorithms.
