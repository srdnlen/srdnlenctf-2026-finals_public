#!/usr/bin/env python3

MASK64 = (1 << 64) - 1
FNV_PRIME = 0x100000001B3

ANSWERS = (
    b"GREYHOUND",
    b"24",
    b"HeavenlyMessenger:OPEN_THE_GATES_OF_DIS",
    b"D4mNAt10",
    b"FCE3DEC2506B2F99",
)


def step(state, byte):
    return ((state ^ byte) * FNV_PRIME) & MASK64


def commit(state, stage, answer):
    state = step(state, 0xA5 ^ (stage * 17))
    state = step(state, len(answer))

    for index, byte in enumerate(answer):
        mask = (stage * 29 + index * 7) & 0xFF
        state = step(state, byte ^ mask)

    return step(state, 0xFF - stage)


def finalize(state):
    state ^= 0x00494E4645524E4F
    state ^= state >> 33
    state = (state * 0xFF51AFD7ED558CCD) & MASK64
    state ^= state >> 33
    state = (state * 0xC4CEB9FE1A85EC53) & MASK64
    state ^= state >> 33
    return state & MASK64


state = 0xCBF29CE484222325
for stage, answer in enumerate(ANSWERS):
    state = commit(state, stage, answer)
    print(f"after stage {stage}: {state:016X}")

print(f"srdnlen{{{finalize(state):016X}}}")

