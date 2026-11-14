from fastecdsa.curve import secp256k1 as Curve
from fastecdsa.point import Point
import xoflib

assert Curve.p.bit_length() <= 256
assert Curve.p % 4 == 3


def point_to_bytes(point: Point) -> bytes:
    """ Converts a Point to its byte representation. """
    if not isinstance(point, Point):
        raise TypeError("point must be a Point instance")
    return bytes([0x02 ^ (point.y & 1)]) + point.x.to_bytes(32, "little")


def bytes_to_point(data: bytes) -> Point:
    """ Converts bytes to a Point on Courve. """
    if not isinstance(data, bytes):
        raise TypeError("data must be bytes")
    if len(data) != 33:
        raise ValueError(f"data must be 33 bytes for {Curve} point")
    bit = int.from_bytes(data[:1], "little") ^ 0x02
    assert bit in {0, 1}
    x = int.from_bytes(data[1:], "little")
    # y^2 = x^3 + ax + b
    y2 = (pow(x, 3, Curve.p) + Curve.a * x + Curve.b) % Curve.p
    y = pow(y2, (Curve.p + 1) // 4, Curve.p)
    if pow(y, 2, Curve.p) != y2:
        raise ValueError("invalid point encoding")
    if x == 0 and bit == 1:
        raise ValueError("invalid point encoding")
    if (y & 1) != bit:
        y = Curve.p - y
    return Point(x, y, Curve)


def point_from_hash(*data):
    h = xoflib.Shake128()
    for d in data:
        if isinstance(d, Point):
            h.absorb(bytes(d))
        elif isinstance(d, int):
            h.absorb(d.to_bytes(32, 'big'))
        elif isinstance(d, bytes):
            h.absorb(d)
        elif isinstance(d, str):
            h.absorb(d.encode())
        else:
            raise ValueError(f"Unsupported data type {type(d)} for hashing")
    xof = h.finalize()
    while True:
        bit = xof.read(1)[0] & 1
        try:
            return bytes_to_point(bytes([0x02 ^ bit]) + xof.read(32))
        except ValueError:
            pass


q = Curve.q
G = Curve.G
O = Curve.G * 0
