#!/usr/bin/env python3

MASK32 = 0xFFFFFFFF
SBOX = (
    0xC, 0x5, 0x6, 0xB, 0x9, 0x0, 0xA, 0xD,
    0x3, 0xE, 0xF, 0x8, 0x4, 0x7, 0x1, 0x2,
)
MASKED_KEY = bytes.fromhex(
    "3F 3B DB F2 F8 80 86 B0 BC 49 59 05 0B 7A 69 5A"
)


def rol32(value, bits):
    bits &= 31
    if bits == 0:
        return value & MASK32
    return ((value << bits) | (value >> (32 - bits))) & MASK32


def recover_key():
    return bytes(
        byte ^ ((0x6D + index * 17) & 0xFF)
        for index, byte in enumerate(MASKED_KEY)
    )


def load_key32(key, offset):
    return int.from_bytes(
        bytes(key[(offset + index) & 15] for index in range(4)),
        "big",
    )


def round_key(key, round_number):
    k0 = load_key32(key, round_number)
    k1 = load_key32(key, round_number + 4)
    return k0 ^ rol32(k1, round_number % 13)


def round_function(value, key_word, round_number):
    value = (value ^ key_word ^ (round_number * 0x9E3779B9)) & MASK32
    substituted = 0
    for nibble in range(8):
        substituted |= SBOX[(value >> (nibble * 4)) & 0xF] << (nibble * 4)

    return (
        rol32(substituted, (round_number % 7) + 3)
        + ((key_word << 1) & MASK32)
        + 0x6C756366
    ) & MASK32


def encrypt(block, key):
    left = int.from_bytes(block[:4], "big")
    right = int.from_bytes(block[4:], "big")

    for round_number in range(16):
        left, right = (
            right,
            left ^ round_function(right, round_key(key, round_number), round_number),
        )

    return left.to_bytes(4, "big") + right.to_bytes(4, "big")


def decrypt(block, key):
    left = int.from_bytes(block[:4], "big")
    right = int.from_bytes(block[4:], "big")

    for round_number in range(15, -1, -1):
        left, right = (
            right ^ round_function(left, round_key(key, round_number), round_number),
            left,
        )

    return left.to_bytes(4, "big") + right.to_bytes(4, "big")


key = recover_key()
plaintext = b"SEE*STAR"
ciphertext = encrypt(plaintext, key)

assert decrypt(ciphertext, key) == plaintext
print(f"Key: {key.decode('ascii')}")
print(f"Ciphertext: {ciphertext.hex().upper()}")

