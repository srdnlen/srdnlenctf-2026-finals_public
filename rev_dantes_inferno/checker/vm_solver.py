#!/usr/bin/env python3

from itertools import product

ALPHABET = bytes(range(0x20, 0x7F))
MASK64 = (1 << 64) - 1

GUARD_META = 0x534255AF
GUARD = bytes((
    0x8E, 0x32, 0x3A, 0xC2, 0x69, 0x0F,
    0xCD, 0x9D, 0x87, 0x39, 0xC7, 0x51,
))
TARGET_HASH = 0xAA18950B28077EAA


def rol8(value, bits):
    bits &= 7
    return ((value << bits) | (value >> ((8 - bits) & 7))) & 0xFF


def sbox(value):
    return (value * 29 + 37) & 0xFF


def fnv1a_step(value, byte):
    return ((value ^ byte) * 0x100000001B3) & MASK64


def vm_hash(candidate):
    state = 0xCBF29CE484222325

    for byte in candidate:
        state = fnv1a_step(state, byte)

    return fnv1a_step(state, len(candidate))


def shard_value(index):
    length = (GUARD_META ^ 0x534255A7) & 0xFF
    slot = (index * 3 + 2) % length
    return GUARD[slot] ^ ((0xA5 + slot * 13) & 0xFF)


def shard_mask(index):
    return rol8(0x1F, index * 3 + 1)


def transformed_byte(index, value):
    return rol8(
        (sbox(value ^ ((0x31 + index * 11) & 0xFF))
         + 0x5C + index * 17) & 0xFF,
        (index % 7) + 1,
    )


def memory_seal_matches(candidate):
    checksum = 0x42
    length = len(candidate)

    for index, current in enumerate(candidate):
        difference = transformed_byte(index, current) ^ shard_value(index)
        if difference & shard_mask(index):
            return False

        checksum = (rol8(checksum ^ current, 1) + index * 9) & 0xFF

    return checksum == (GUARD[length] ^ 0x6D)


def solve():
    length = (GUARD_META ^ 0x534255A7) & 0xFF
    allowed = []

    for index in range(length):
        allowed.append(bytes(
            value for value in ALPHABET
            if not ((transformed_byte(index, value) ^ shard_value(index))
                    & shard_mask(index))
        ))

    sealed = []
    matches = []
    for values in product(*allowed):
        candidate = bytes(values)
        if memory_seal_matches(candidate):
            sealed.append(candidate)
            if vm_hash(candidate) == TARGET_HASH:
                matches.append(candidate)

    if len(sealed) != 35 or len(matches) != 1:
        raise SystemExit(
            f"Seal={len(sealed)}, full={len(matches)}"
        )

    print(f"Candidates: {len(sealed)}")
    print(f"Matches: {len(matches)}")
    print(f"Answer: {matches[0].decode('ascii')}")


if __name__ == "__main__":
    solve()
