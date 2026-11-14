from sage.all import *
from pwn import process, remote
from bulletproofs import (
    n, p, G, H, O, point_to_bytes, 
    BulletproofsProofType, InnerProductArgument, RangeProof, Transcript
)
import os, secrets, json

io = process(["python3", "server.py"])
sid = bytes.fromhex(
    io.recvline_contains(b"Here's your session id: ").decode().split(": ").pop()
)
transcript = Transcript(sid)

Fp = GF(p)
R = PolynomialRing(Fp, ["v"] + [f"aL_{i}" for i in range(n)] + [f"aR_{i}" for i in range(n)])
v = R.gen(0)
aL = list(R.gens()[1:n + 1])
aR = list(R.gens()[n + 1:])

RX = R["X"]
X = RX.gen()

y = X
z = y * transcript.a + transcript.b

l, r = [], []
y_, z2 = 1, z**2
for i in range(n):
    l.append(aL[i] - z)
    r.append(y_ * (aR[i] + z) + (z2 * 2**i))
    y_ = (y_ * y)
t0 = 0
for l0, r0 in zip(l, r):
    t0 += l0 * r0

delta = lambda y, z: (z - z**2) * sum(y**i for i in range(n)) - z**3 * (2**n - 1)
# we want to satisfy each equation in the coefficients of X^i
zeros = list(v * z**2 + delta(y, z) - t0)

vars = set()
for z in zeros:
    vars.update(z.variables())
assert len(vars) == 1 + 2 * n

mons = set()
for z in zeros:
    for mon in z.monomials():
        if mon.degree() >= 2:
            mons.add(mon)
assert mons == {aL[i] * aR[i] for i in range(n)}

assert len(zeros) == n + 1
# by fixing n variables we should have enough free variables for a linear solution
subs = {aL[i]: secrets.randbelow(p) for i in range(n)}

S = [z.subs(subs) for z in zeros]
M, Ys = Sequence(S).coefficients_monomials(sparse=False)

M, rhs = M[:, :-1], -M.column(-1)
sol = M.solve_right(rhs)

subs.update({Y: y for Y, y in zip(Ys, map(int, sol))})
assert subs[v].bit_length() > 64
assert all(z.subs(subs) == 0 for z in zeros)


class MyRangeProof(RangeProof):
    @classmethod
    def prove(
        cls, v: int, gamma: int, 
        transcript: Transcript, *, serialize: bool = False,
        a=None
    ) -> BulletproofsProofType:
        if n > 64:
            raise ValueError("n must be at most 64")
        if n & (n - 1) != 0:
            raise ValueError("n must be a power of 2")
        
        V = v * G + gamma * H

        if a is None:
            aL = [(v >> i) & 1 for i in range(n)]
            aR = [(aL[i] - 1) % p for i in range(n)]
        else:
            aL, aR = a
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


gamma = secrets.randbelow(p)
a = (
    [int(subs[aL[i]]) for i in range(n)],
    [int(subs[aR[i]]) for i in range(n)],
)
proof = MyRangeProof.prove(subs[v], gamma, Transcript(sid), a=a, serialize=True)
V = subs[v] * G + gamma * H
assert MyRangeProof.verify(V, proof, Transcript(sid), deserialize=True)

broken_soundness = {
    "v": subs[v], "gamma": gamma, "proof": proof
}
io.sendlineafter(
    b"Give me your proof for an invalid range: ", 
    json.dumps(broken_soundness).encode()
)

try:
    io.interactive()
except (KeyboardInterrupt, EOFError):
    pass
finally:
    io.close()
