#!/usr/bin/env python3

MASK64 = (1 << 64) - 1
TARGET_MASKED = (0x0D5533A9, 0xCE70F311)
TARGET_MASK = (0xA6D9345C, 0x37C2E19A)

FILES = (
    ("limbo/register.txt", 0x13, "EB B5 4A 44 F6 A9 CB 0D E3 1B C1 40 AF 54 1E 13"),
    ("styx/mud.bin", 0x37, "C8 D2 84 A7 7A 0D 8A 10 AF 02 A2"),
    (
        "dis/messenger.sig",
        0x59,
        "3D 49 A6 6C 6E D8 4E 08 EC 55 E5 B1 5E BA 64 2A "
        "3C 76 6D D4 A3 77 C8 BA 30 D1 5E 56 5C 9F C7 CA "
        "1A 81 9A A4 26 14 B4",
    ),
    (
        "dis/heresy.log",
        0x7B,
        "DF 2D 3F 44 DB A6 B9 08 4C E5 03 C5 F9 4F 94 D8 "
        "8C EB 53 94 D6 80 B6 46 E0",
    ),
    (
        "dis/walls.txt",
        0x9D,
        "40 78 E0 3F 56 DB F5 16 67 37 EF 17 45 81 9D 91 13 1F F1 96",
    ),
)


def ror8(value, bits):
    bits &= 7
    return ((value >> bits) | (value << ((8 - bits) & 7))) & 0xFF


def rol64(value, bits):
    bits &= 63
    return ((value << bits) | (value >> (64 - bits))) & MASK64


def decode(encoded, nonce):
    output = bytearray()
    for index, byte in enumerate(encoded):
        rotation = ((index + nonce) % 7) + 1
        mask = (0xA7 + nonce * 0x1D + index * 0x3B) & 0xFF
        output.append(ror8(byte, rotation) ^ mask)
    return bytes(output)


def fingerprint(data):
    value = 0x4449544553545958
    for index, byte in enumerate(data):
        value = rol64(value ^ byte, 11)
        value = (value + (index + 1) * 0x9E3779B185EBCA87) & MASK64
        value ^= value >> 17
    return value & MASK64


def target_fingerprint():
    low = TARGET_MASKED[0] ^ TARGET_MASK[0]
    high = TARGET_MASKED[1] ^ TARGET_MASK[1]
    return (high << 32) | low


def main():
    target = target_fingerprint()
    matches = []
    print(f"Requested fingerprint: {target:016X}\n")

    for path, nonce, encoded_hex in FILES:
        body = decode(bytes.fromhex(encoded_hex), nonce)
        body_fingerprint = fingerprint(body)
        print(f"{body_fingerprint:016X}  {path:20}  {body.decode('ascii')}")
        if body_fingerprint == target:
            matches.append(body)

    if len(matches) != 1:
        raise SystemExit(f"Expected one matching record, found {len(matches)}")

    print(f"\nAnswer: {matches[0].decode('ascii')}")


if __name__ == "__main__":
    main()
