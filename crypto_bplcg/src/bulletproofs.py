# Implementation of Bulletproofs following the notation of
# https://eprint.iacr.org/2017/1066.pdf

from ecc import q as p, G as H, O, Point, point_from_hash, point_to_bytes, bytes_to_point
from pedersen import B as G
from transcript import Transcript
import secrets

BulletproofsProofType = tuple[
    tuple[Point, Point, Point, Point, list[tuple[Point, Point]]],
    tuple[int, int, int, tuple[int, int]]
]

n = 64


class InnerProductArgument:
    """
    Protocol 2 in https://eprint.iacr.org/2017/1066.pdf
    """

    @classmethod
    def prove(
        cls, 
        G: list[Point], H: list[Point], 
        U: Point, P: Point, 
        a: list[int], b: list[int],
        transcript: Transcript
    ) -> tuple[list[tuple[Point, Point]], tuple[int, int]]:
        if not (len(G) == len(H) == len(a) == len(b)):
            raise ValueError("Invalid input lengths")
        n = len(G)
        if n == 1:
            return [], (a[0], b[0])
        
        cL = sum(a[i] * b[n // 2 + i] for i in range(n // 2)) % p
        cR = sum(a[n // 2 + i] * b[i] for i in range(n // 2)) % p
        L = sum((a[i] * G[n // 2 + i] for i in range(n // 2)), O) + \
            sum((b[n // 2 + i] * H[i] for i in range(n // 2)), O) + \
            cL * U
        R = sum((a[n // 2 + i] * G[i] for i in range(n // 2)), O) + \
            sum((b[i] * H[n // 2 + i] for i in range(n // 2)), O) + \
            cR * U

        transcript.append(L, R)
        x = transcript.challenge(p)
        x_inv = pow(x, -1, p)

        G = [x_inv * G[i] + x * G[n // 2 + i] for i in range(n // 2)]
        H = [x * H[i] + x_inv * H[n // 2 + i] for i in range(n // 2)]
        P = pow(x, 2, p) * L + P + pow(x_inv, 2, p) * R
        a = [(a[i] * x + a[n // 2 + i] * x_inv) % p for i in range(n // 2)]
        b = [(b[i] * x_inv + b[n // 2 + i] * x) % p for i in range(n // 2)]

        proof, final = cls.prove(G, H, U, P, a, b, transcript)
        return [(L, R)] + proof, final
    
    @staticmethod
    def verify(
        G: list[Point], H: list[Point], 
        U: Point, P: Point, 
        proof: list[tuple[Point, Point]], 
        final: tuple[int, int],
        transcript: Transcript
    ) -> bool:
        if not (len(G) == len(H)):
            raise ValueError("Invalid input lengths")
        n = len(G)
        if n & (n - 1) != 0:
            raise ValueError("Length of G and H must be a power of 2")
        if n.bit_length() - 1 != len(proof):
            raise ValueError("Invalid proof length")

        a, b = final
        if not all(isinstance(L, Point) and isinstance(R, Point) for L, R in proof):
            raise ValueError("L's and R's must be points")    
        if not isinstance(a, int) or not isinstance(b, int):
            raise ValueError("a and b must be integers")
        
        for L, R in proof:
            n = len(G)
            transcript.append(L, R)
            x = transcript.challenge(p)
            x_inv = pow(x, -1, p)

            G = [x_inv * G[i] + x * G[n // 2 + i] for i in range(n // 2)]
            H = [x * H[i] + x_inv * H[n // 2 + i] for i in range(n // 2)]
            P = pow(x, 2, p) * L + P + pow(x_inv, 2, p) * R
        
        return P == a * G[0] + b * H[0] + (a * b % p) * U


class RangeProof:
    """
    Section 4.1 in https://eprint.iacr.org/2017/1066.pdf
    """

    G = [point_from_hash(f"Bulletproofs n={n} G{i}") for i in range(n)]
    H = [point_from_hash(f"Bulletproofs n={n} H{i}") for i in range(n)]
    U = point_from_hash(f"Bulletproofs n={n} U")

    @classmethod
    def prove(
        cls, v: int, gamma: int, 
        transcript: Transcript, *, serialize: bool = False
    ) -> BulletproofsProofType:
        if n > 64:
            raise ValueError("n must be at most 64")
        if n & (n - 1) != 0:
            raise ValueError("n must be a power of 2")
        if v < 0 or v >= 1 << n:
            raise ValueError("Value out of range")
        
        V = v * G + gamma * H

        aL = [(v >> i) & 1 for i in range(n)]
        aR = [(aL[i] - 1) % p for i in range(n)]
        alpha = secrets.randbelow(p)
        A = alpha * H + sum((aL[i] * cls.G[i] + aR[i] * cls.H[i] for i in range(n)), O)

        sL = [secrets.randbelow(p) for _ in range(n)]
        sR = [secrets.randbelow(p) for _ in range(n)]
        rho = secrets.randbelow(p)
        S = rho * H + sum((sL[i] * cls.G[i] + sR[i] * cls.H[i] for i in range(n)), O)

        transcript.append(V, A, S)
        y = transcript.challenge(p)
        z = transcript.challenge(p)

        l, r = [], []
        y_, z2 = 1, pow(z, 2, p)
        for i in range(n):
            l.append([(aL[i] - z) % p, sL[i]])
            r.append([
                (y_ * (aR[i] + z) + (z2 << i)) % p, 
                (y_ * sR[i]) % p
            ])
            y_ = (y_ * y) % p
        t0, t1, t2 = 0, 0, 0
        for (l0, l1), (r0, r1) in zip(l, r):
            t0 = (t0 + l0 * r0) % p
            t1 = (t1 + l0 * r1 + l1 * r0) % p
            t2 = (t2 + l1 * r1) % p

        tau1 = secrets.randbelow(p)
        tau2 = secrets.randbelow(p)
        T1 = t1 * G + tau1 * H
        T2 = t2 * G + tau2 * H

        transcript.append(T1, T2)
        x = transcript.challenge(p)

        l = [(l0 + l1 * x) % p for l0, l1 in l]
        r = [(r0 + r1 * x) % p for r0, r1 in r]
        t_hat = sum(l[i] * r[i] for i in range(n)) % p
        tau_x = (tau2 * x**2 + tau1 * x + z2 * gamma) % p
        mu = (alpha + rho * x) % p

        H_prime = [pow(y, -i, p) * cls.H[i] for i in range(n)]

        delta = ((z - z2) * sum(pow(y, i, p) for i in range(n)) - z * z2 * ((1 << n) - 1)) % p
        assert t_hat * G + tau_x * H == z2 * V + delta * G + x * T1 + pow(x, 2, p) * T2
        P = A + x * S - z * sum(cls.G, O) + \
            sum(((z * pow(y, i, p) + (z2 << i)) % p * H_prime[i] for i in range(n)), O)
        assert P == mu * H + sum((l[i] * cls.G[i] + r[i] * H_prime[i] for i in range(n)), O)
        
        # Protocol 1 in https://eprint.iacr.org/2017/1066.pdf
        transcript.append(tau_x, mu, t_hat)
        x = transcript.challenge(p)

        U = x * cls.U
        P = P - mu * H + t_hat * U
        proof, final = InnerProductArgument.prove(cls.G, H_prime, U, P, l, r, transcript)

        if serialize:
            point_to_hex = lambda x: point_to_bytes(x).hex()
            A, S, T1, T2 = map(point_to_hex, (A, S, T1, T2))
            proof = [(point_to_hex(L), point_to_hex(R)) for L, R in proof]
        
        return (A, S, T1, T2, proof), (tau_x, mu, t_hat, final)

    @classmethod
    def verify(
        cls, V: Point, proof: BulletproofsProofType, 
        transcript: Transcript, *, deserialize: bool = False
    ) -> bool:
        if n > 64:
            raise ValueError("n must be at most 64")
        if n & (n - 1) != 0:
            raise ValueError("n must be a power of 2")
        
        (A, S, T1, T2, proof), (tau_x, mu, t_hat, final) = proof
        if n.bit_length() - 1 != len(proof):
            raise ValueError("Invalid proof length")

        if deserialize:
            hex_to_point = lambda x: bytes_to_point(bytes.fromhex(x))
            A, S, T1, T2 = map(hex_to_point, (A, S, T1, T2))
            proof = [(hex_to_point(L), hex_to_point(R)) for L, R in proof]

        if not all(isinstance(P, Point) for P in (A, S, T1, T2)):
            raise ValueError("A, S, T1, T2 must be points")
        if not all(isinstance(x, int) for x in (tau_x, mu, t_hat)):
            raise ValueError("tau_x, mu, t_hat must be integers")

        transcript.append(V, A, S)
        y = transcript.challenge(p)
        z = transcript.challenge(p)
        z2 = pow(z, 2, p)
        H_prime = [pow(y, -i, p) * cls.H[i] for i in range(n)]

        transcript.append(T1, T2)
        x = transcript.challenge(p)

        delta = ((z - z2) * sum(pow(y, i, p) for i in range(n)) - z * z2 * ((1 << n) - 1)) % p
        if t_hat * G + tau_x * H != z2 * V + delta * G + x * T1 + pow(x, 2, p) * T2:
            return False
        P = A + x * S - z * sum(cls.G, O) + \
            sum(((z * pow(y, i, p) + (z2 << i)) % p * H_prime[i] for i in range(n)), O)
        
        # Protocol 1 in https://eprint.iacr.org/2017/1066.pdf
        transcript.append(tau_x, mu, t_hat)
        x = transcript.challenge(p)

        U = x * cls.U
        P = P - mu * H + t_hat * U
        return InnerProductArgument.verify(cls.G, H_prime, U, P, proof, final, transcript)
        

def prove(
        v: int, gamma: int, 
        sid: bytes, *, serialize: bool = False
    ) -> BulletproofsProofType:
    transcript = Transcript(sid)
    return RangeProof.prove(v, gamma, transcript, serialize=serialize)


def verify(
        V: Point, proof: BulletproofsProofType, 
        sid: bytes, *, deserialize: bool = False
    ) -> bool:
    transcript = Transcript(sid)
    return RangeProof.verify(V, proof, transcript, deserialize=deserialize)


if __name__ == "__main__":
    sid = secrets.token_bytes(32)
    v = secrets.randbits(n)
    gamma = secrets.randbelow(p)
    proof = prove(v, gamma, sid, serialize=True)
    assert verify(v * G + gamma * H, proof, sid, deserialize=True)
