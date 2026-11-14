from bulletproofs import p, G, H, n, verify
import os, json

sid = os.urandom(32)
print(f"Here's your session id: {sid.hex()}")

broken_soundness = json.loads(input("Give me your proof for an invalid range: "))
v = broken_soundness["v"]
gamma = broken_soundness["gamma"]
proof = broken_soundness["proof"]

assert isinstance(v, int) and isinstance(gamma, int)
assert 0 <= v < p and 0 <= gamma < p
assert not 0 <= v < 1 << n
V = v * G + gamma * H
assert verify(V, proof, sid, deserialize=True)

print(os.getenv('FLAG', 'srdnlen{this_is_a_fake_flag}'))
