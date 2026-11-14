from numpy.fft import fft, ifft
import os, numpy as np

n = 509
q = 2**11

one = np.zeros(n, dtype=np.int64)
one[0] = 1


#### Operations over Zq[x] / (x^n - 1) using FFT


def mul(a: np.ndarray, b: np.ndarray) -> np.ndarray:
    global n, q
    assert len(a) == len(b) == n
    k = 1 << (n.bit_length() + 1)
    # c(x) = a(x) * b(x)
    c = ifft(fft(a, k) * fft(b, k)).real.round().astype(np.int64) % q
    # c(x) mod x^n - 1
    c = (c[:n] + c[n:2 * n]) % q
    return c


def inverse(a: np.ndarray) -> np.ndarray:
    global n, q, one
    assert len(a) == n
    assert q & (q - 1) == 0
    
    a0 = a & 1
    # assuming n is prime and x^n + 1 factors into 
    # (x + 1) * (x^{n - 1} + ... + x + 1) modulo 2
    if np.sum(a0 == 1) & 1 == 0 or np.sum(a0 == 1) in {0, n}:
        raise ValueError("element is not invertible")

    # inverse of a(x) modulo 2
    a0_pow = mul(a0, a0) & 1
    a0_inv = a0_pow.copy()
    for _ in range(n - 3):
        a0_pow = mul(a0_pow, a0_pow) & 1
        a0_inv = mul(a0_inv, a0_pow) & 1
    assert np.all(mul(a0, a0_inv) & 1 == one)

    # inverse of a(x) modulo q
    a_inv = a0_inv.copy()
    k = 1
    while k < q.bit_length() - 1:
        k = min(2 * k, q.bit_length() - 1)
        e = (one - mul(a, a_inv)) % 2**k
        a_inv = (a_inv + mul(a_inv, e)) % 2**k
    assert np.all(mul(a, a_inv) == one)
    
    return a_inv


#### NTRUEncrypt


class NTRUEncrypt:
    @staticmethod
    def ternary() -> np.ndarray:
        global n
        a = np.frombuffer(os.urandom(8 * n), dtype=np.int64)
        a = a % 3 - np.ones_like(a)
        return a

    @classmethod
    def keygen(cls) -> tuple[np.ndarray, np.ndarray]:
        global q
        while True:
            try:
                f = one + 3 * cls.ternary()
                fq = inverse(f)
                break
            except ValueError:
                pass
        g = cls.ternary()
        h = 3 * mul(fq, g) % q
        return f, h

    @classmethod
    def encrypt(cls, h: np.ndarray, m: np.ndarray) -> np.ndarray:
        global q
        r = cls.ternary()
        e = (mul(h, r) + m) % q
        return e

    @classmethod
    def decrypt(cls, f: np.ndarray, e: np.ndarray) -> np.ndarray:
        global q
        a = mul(f, e)
        m = np.where(a <= q // 2, a, a - q) % 3
        return m


if __name__ == "__main__":
    flag = os.getenv('FLAG', 'srdnlen{this_is_a_fake_flag}').encode()
    assert len(flag) <= n // 8

    sk, pk = NTRUEncrypt.keygen()

    while True:
        data = bytes.fromhex(input("Enter your query: "))
        if not data:
            break
        assert len(data) == 2 * n
        e = np.frombuffer(data, dtype=np.int16).astype(np.int64) % q
        r = np.random.randint(0, q, n)
        m = NTRUEncrypt.decrypt(sk, e + r)
        response = (
            r.astype(np.int16).tobytes().hex(),
            m.astype(np.int16).tobytes().hex()
        )
        print(f"Response: {response}")

    m = np.frombuffer(flag, dtype=np.uint8)
    m = np.unpackbits(m, count=n, bitorder="little")
    e = NTRUEncrypt.encrypt(pk, m)
    print(f"Encrypted flag: {e.astype(np.int16).tobytes().hex()}")
