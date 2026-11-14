from randcrack import RandCrack
from gf2bv.crypto.mt import MT19937
from gf2bv import LinearSystem
import numpy as np

a = np.random.randint(0, 2**32, 624)

rc = RandCrack()

for x in map(int, a):
    rc.submit(x)

t = rc.predict_getrandbits(32)
guess = np.random.randint(0, 2**32)
assert t == guess

state = np.random.get_state()

bits = 11
mask = 2**bits - 1
a = np.random.randint(0, 2**bits, 2**12)

assert a.tolist() == [rc.predict_getrandbits(32) & mask for _ in range(len(a))]

lin = LinearSystem([32] * 624)
mt = MT19937(lin.gens())

zeros = []
for x in map(int, a):
    zeros.append((mt.getrandbits(32) & mask) ^ x)

sol = lin.solve_one(zeros)
