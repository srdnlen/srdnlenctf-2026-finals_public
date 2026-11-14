from ecc import q, G, Point, point_from_hash
import secrets

B = point_from_hash("Pedersen Blinding Generator")


def commit(v: int, r: int = None) -> tuple[Point, int]:
    if r is None:
        r = secrets.randbelow(q)
        return v * B + r * G, r
    return v * B + r * G


def verify(C: Point, v: int, r: int) -> bool:
    return C == v * B + r * G
