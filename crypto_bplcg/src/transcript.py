from ecc import q, Point, point_to_bytes
import hashlib

__all__ = ["Transcript"]


class Transcript:
    def __init__(self, sid: bytes) -> None:
        self.a = int.from_bytes(hashlib.sha3_256(sid + b"a").digest(), "big") % q
        self.b = int.from_bytes(hashlib.sha3_256(sid + b"b").digest(), "big") % q
        self.s = int.from_bytes(sid, "big") % q

    def lcg(self):
        self.s = (self.s * self.a + self.b) % q

    def append(self, *args) -> None:
        for arg in args:
            if isinstance(arg, int):
                self.s += arg % q
            elif isinstance(arg, Point):
                self.s += int.from_bytes(point_to_bytes(arg), "big") % q
            else:
                raise ValueError(f"Unsupported data type {type(arg)} for append")
            self.lcg()

    def challenge(self, mod: int = q) -> int:
        chall = self.s % mod
        self.lcg()
        return chall
