from pwn import process, remote, context
from server import n, q, one, NTRUEncrypt
from gf2bv.crypto.mt import MT19937
from gf2bv import LinearSystem
import ast, numpy as np, tqdm, math

context.log_level = "info"
io = process(["python3", "server.py"])


def query(e: np.ndarray):
    global io
    data = e.astype(np.int16).tobytes().hex()
    io.sendlineafter(b"Enter your query: ", data.encode())
    data = ast.literal_eval(
        io.recvline_contains(b"Response: ").decode().split(": ").pop()
    )
    r, m = map(bytes.fromhex, data)
    r = np.frombuffer(r, dtype=np.int16).astype(np.int64)
    m = np.frombuffer(m, dtype=np.int16).astype(np.int64)
    return r, m


lin = LinearSystem([32] * 624)
mt = MT19937(lin.gens())
mask = 2**(q.bit_length() - 1) - 1

nqueries = math.ceil((624 * 32) / ((q.bit_length() - 1) * n))
nqueries *= 2  # otherwise we do not recover a unique state
rs = []
for _ in tqdm.trange(nqueries):
    r, _ = query(np.zeros(n, dtype=np.int64))
    rs.append(r)

zeros = []
for r in rs:
    for x in map(int, r):
        zeros.append(x ^ (mt.getrandbits(32) & mask))
sol = lin.solve_one(zeros)
assert sol is not None

mt = MT19937(sol)
for r in rs:
    for i in range(n):
        assert r[i] == mt.getrandbits(32) & mask

r_full = np.array([mt.getrandbits(32) for _ in range(n)], dtype=np.int64)
r = r_full & mask
e = (one * pow(3, -1, q) - r) % q
r_, m = query(e)
assert np.all(r == r_)

io.sendline(b"")
data = bytes.fromhex(
    io.recvline_contains(b"Encrypted flag: ").decode().split(": ").pop()
)
e = np.frombuffer(data, dtype=np.int16).astype(np.int64) % q

m = np.where(m <= 1, m, m - 3)
sk = 3 * (m - one * m[0]) + one
# the first entry is 1 modulo 3 plus a ternary digit multiplied by 3
for x in (-2, 1, 4):
    sk[0] = x
    m = NTRUEncrypt.decrypt(sk, e)
    flag = np.packbits(m, bitorder="little").tobytes()
    try:
        print(flag.strip().decode())
        break
    except UnicodeDecodeError:
        continue

io.close()
