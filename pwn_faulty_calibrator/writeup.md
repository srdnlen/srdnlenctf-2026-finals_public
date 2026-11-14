# Faulty Calibrator

- **CTF:** Srdnlen CTF 2026 Finals
- **Category:** pwn
- **Difficulty:** Jeopardy
- **Authors:** [@0x41lby] (Alberto Perra)

## Description

- **Platform:** Linux
- **Glibc Version**: 2.42

## Security Checks
```
Arch:       amd64-64-little
RELRO:      Full RELRO
Stack:      Canary found
NX:         NX enabled
PIE:        No PIE (0x400000)
Stripped:   No
```
## Overview

This challenge revolves around allocating chunks corresponding to specific ammunition types, so different ammunition allocates chunks of different sizes. More specifically, this challenge revolves around two main ammunition types:
- __HandgunAmmo__: requires the allocation of a chunk of size __0x210__, resulting in a chunk of size __0x220__.
- __ShotgunShellAmmo__: requires the allocation of a chunk of size __0x428__, resulting in a chunk of size __0x430__.
- __RifleRoundAmmo__: requires the allocation of a chunk of size __0x638__, resulting in a chunk of size __0x640__.

Objects belonging to the last two classes inherit a virtual method from the __LongGunAmmo__ class, so they require __8__ extra bytes to write a pointer to a VTable in the chunk.
The vulnerability is an __Off-by-Null__ vulnerability within the add_description function that allows an attacker to compromise the prev_inuse field of the next chunk.

All the ammunitions have a certain price and the challenge itself provides a budget, which at the end, if all is done correctly, will be exactly __0__.

## Solution

By creating and destroying __HandGunAmmo__ objects, it is possible to leak the heap's base address, and since chunks are small in size, it is possible to bypass safe linking by only having the obfuscated pointer available using one of the typical de-obfuscation formulas, for example:
```
def decoding_fd(encoded_fd):
    encoded_fd ^= encoded_fd >> 12
    encoded_fd ^= encoded_fd >> 24
    return encoded_fd
```
Next, we can use __House of Einherjar__ to get the program to allocate a huge chunk. We allocate two chunks, __A__ and __B__.

Exploiting the __Off-by-One__, we write to __A__ and use the __NULL__ byte to compromise the __prev_inuse__ field of chunk __B__, which, besides forcing the heap to consider chunk __A__ as free, will also reduce the size of __B__ inevitably to __0x400__. We must also consider the libc check performed during the backward consolidation process:
```
A->fd->bk == A
A->bk->fd == A
```
Which can be bypassed simply setting the following in A:
```
A->fd = A
A->bk = A
```
And therefore the final shape of the write on __A__ will be:
```
payload = flat(
        0x421,                  # fake size at fake+0x08
        fake,                   # fd at fake+0x10
        fake,                   # bk at fake+0x18
        0x0,                    # fd_nextsize
        0x0,                    # bk_nextsize
        b"A" * (0x418 - 0x28),  # padding to reach prev_size
        0x420                   # B->prev_size
    )
```
At this point, we write padding bytes corresponding to the new size into __B__ and create a fake chunk __B1__ inside __B__, writing fake metadata to make the remainder consistent with the heap state during the forward consolidation process. This way, the heap treats __B1__ as a busy chunk:
```
payload = flat(
        b"B" * 0x3e8,           # padding to reach B1
        0x400,                  # prev_size of B
        0x31                    # size of the remainder
    )
```
Then we free __B__ to trigger the vulnerability. This is the theoretical process; in reality, it will need to be executed within a loop because the freed chunk __B__ will not be large enough to end up in the largebins, but will end up in the tchache, which must be filled first.

Once the previous step is complete, if we allocate a __RifleRoundAmmo__ object, it won't be any large chunk, but rather the combination of the previous chunks __A__ and __B__. However, we still have a reference to chunk A, so it's possible to write into __A__ and, more specifically, compromise the vptr of the virtual fire() method. At this point, we create a fake VTable and overwrite the vptr of the huge chunk returned so the vptr points to __win()__ instead of __fire()__.
```
payload = flat(
    0x600,
    fake + 0x100,               # fake is the address of the chunk
    b"C" * (0x100 - 0x18),
    win
)
```
At this point, we simply call __fire()__ and instead of executing the method, the win function will spawn a shell. Finally, we can simply retrive the flag:
```
srdnlen{0fF_bY_oN3_bUll3t_f0R_mY_Vt4Ble}
```





