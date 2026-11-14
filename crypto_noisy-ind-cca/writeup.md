# Noisy IND-CCA

- **Category:** Crypto
- **Solves:** 1
- **Tag:** NTRU, Mersenne Twister

## Description

Just a noisy IND-CCA oracle.

## Details

We are given an IND-CCA oracle of NTRUEncrypt that masks our queries $e$ with uniform ring elements $r$. Thus, the response of the oracle is $\mathsf{Decrypt}(e + r)$. The objective is to use the oracle to gather enough information and then recover the flag encrypted using NTRUEncrypt.

## Solution

Assuming $r$ is unpredictable, the challenge becomes learning something about $\mathsf{sk}$ given only decryptions of uniform elements $e + r$, which should be unfeasible for NTRUEncrypt. Thus, we may assume that $r$ is predictable and try to investigate this path.

The ring element $r$ is generated using `np.random.randint(0, q, n)`, where `q = 1 << 11` and `n = 509`. Internally `np.random` uses Mersenne Twister. Thus, if `np.random.randint` does not reject outputs from MT, the recovery of the state should be straightforward.

We could check how the output of MT is parsed by `np.random.randint` by delving into the `numpy` src. Instead, we guess :slot_machine: how it might work under the hood.

First of all, `x = np.random.randint(0, q)` is exclusive, thus `0 <= x < q`. There are mainly two methods we could count the amount of bits needed to output `x`:
1. either we take `k = q.bit_length()` like `random.randrange`, or
2. we take `k = (q - 1).bit_length()`.

The first gives MT outputs that are out of range $1 / 2$ of the times, while the second is the best one if we want a simple solve. Next, there are two main ways of restricting a 32-bit MT output `y` to a `k`-bit output `x`:
1. `x = y >> (32 - k)`, or
2. `x = y & ((1 << k) - 1)`.

Turns out the second option, in both cases, is the right answer.

To recover the MT state we use [gf2bv](https://github.com/maple3142/gf2bv) by maple3142. Recovering $\mathsf{sk}$ is easy, as we simply need to send $e = 3^{-1} - r \pmod{q}$ and get

$$\mathsf{Decrypt}(3^{-1}) = \lceil f / 3 \pmod{q} \rfloor_p,$$

from which we can recover $\mathsf{sk}$. See the actual implementation in [solve.py](./src/solve.py) for more details.

