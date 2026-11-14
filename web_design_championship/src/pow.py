"""
Proof of work (hashcash, SHA-1).
"""
import hashlib
import secrets

BITS = 26


def new_resource() -> str:
    """A fresh per-client challenge string (4 random bytes, hex)."""
    return secrets.token_hex(4)


def check(stamp: str, resource: str, bits: int = BITS) -> bool:
    """Validate a hashcash stamp against ``resource`` at ``bits`` difficulty."""
    stamp = stamp.strip()
    if not stamp.startswith("1:"):
        return False
    parts = stamp.split(":")
    if len(parts) < 7:
        return False
    _version, claim, _date, res, _ext, _rand, _counter = parts[:7]
    try:
        claim_bits = int(claim)
    except ValueError:
        return False
    if res != resource:
        return False
    if bits > claim_bits:
        return False
    hex_zeros = claim_bits // 4
    return hashlib.sha1(stamp.encode()).hexdigest().startswith("0" * hex_zeros)
