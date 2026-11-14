# Eureka

- **Category:** Crypto
- **Solves:** 1
- **Tag:** Hardware, AES

## Description

Recently my friend started learning about block ciphers. He's convinced it's not a big problem to do some little changes since it's all super secure. I know he's wrong but he's also very stubborn so, to show me he's right, he prepared a custom algorithm to put in a FPGA and he asked me to try to break it. I'm not the best crypter so I need help, can you recover the flag?

## Details

In order to solve this challenge, players had to interact with a physical board, which was running an AES encryption oracle for a custom mode of operation. The players are also given the ciphertext of the flag, encrypted in the same way, and the objective is to use the provided oracle to decrypt it.

## Solution - Hardware

We use a Cmod A7 35T with a clock of 12MHz.

Here's what the verilog does:
- AesCore.sv: implements AES-128
- MixColumns.sv/InvMixColumns.sv: implement MixColumns/InvMixColumns
- SboxMem.sv: prepares Sbox and InvSbox in memory as LUTs
- KeyScheduler.sv: prepares all roundkeys needed 
- Crypto.sv: implmenets a custom mode of operations explained in Solution - Crypto
- uartTX.sv/uartRX.sv: implement a UART communication
- bufferTX.sv/bufferRX.sv: bufferize user's input and encryption result
- Button.sv: manages the button
- Top.sv: top module, connects every module
- Top.xdc: costraints file to program the FPGA

uartRX receives a bit at a time at 115200 bit/s (12MHz/104), after it got a full byte it sends it to bufferRX. bufferRX is a 16*16 byte matrix that contains up to 16 blocks of 16 bytes. If the buffer is full it ignores new bytes until there is room again. When the button managed by Button is pressed the block stored are encrypted. If the buffer is empty it's the flag the one that gets encrypted. When the board eneters the state of encryption bufferRX doesn't accept any new bytes until it's done. Crypto asks a block at a time until the buffer is empty. Once a block is encrypted is put in bufferTX. When Crypto finishes is job bufferRX can store new bytes while bufferTX starts sending it's bytes to uartTX to transmit them.
There is no pipeline and the communciation between modules happens through handshakes to synchronize them.

Now the aim is to connect our pc to the FPGA. In the .xdc file we can see that RX and TX pin are L3 and M3, in the official datasheet of the FPGA the pins are named with a number from 1 and 48. To see the mapping between numbers and code names we have to go to the github page of Digilent (the company that produces the FPGA), go to the digilent-xdc repo and then to the repo named after our board. There is a general .xdc file with the complete mapping. It's also possible to retrieve this info googling hard enough. From there we know that the pin are 1 and 2 and we can communicate with the board. To test it we can just press the button and see if we get the encrypted flag. Now we can focus on the custom mode of operation and try to break it.

## Solution - Crypto

Now that we understood how it works, we notice that the custom mode of operation is built in such a way that an attacker can achieve arbitrary ECB encryption and decryption by structuring the blocks in a certain way.

In the first stage of the encryption, each block is ECB encrypted and then xored with the previous block (plaintext) and a fixed value `CST(i+1)`, where `i` is the index of the block, then in the second phase each block is ECB decrypted and xored with the next one (the last one gets xored with the nonce).

Knowing all this, it's pretty intuitive that if we have a message like `[CST(2), ZERO_BLOCK, PT_BLOCK]`, when the zero block gets encrypted its ciphertext gets xored with the previous block (`CST(2)`) and with `CST(2)`, so it stays the same, and then when it's decrypted and xored with the next block we'll have `ZERO_BLOCK ^ (E(PT_BLOCK) ^ ZERO_BLOCK ^ CST(3))`, so we can easily get the value of `E(PT_BLOCK)` by xoring that block with the known value `CST(3)`. We can find that block at index 1, the same at which the zero block was.

Now that we have an ECB encryption oracle, we can use it to get an ECB decryption oracle. The way we do this is by getting the ECB encryption of the zero block, let's call it `E0`, and then we build a message like `[E0 ^ CST(2) ^ CT_BLOCK, ZERO_BLOCK, ZERO_BLOCK]`, because after the first phase of the encryption, the first zero block becomes `CT_BLOCK`, and then it gets decrypted and xored with the next block, which is `E0 ^ CST(3)`, a known value, so in the block at index 1 of the result we have `D(CT_BLOCK) ^ E0 ^ CST(3)`, hence we know the value of `D(CT_BLOCK)`.

At this point we can just repeat the same operations that the provided `decrypt` function does and get the flag.

```
srdnlen{d1ffus10n_c0nst4nts_l0l}
```

## Full Solve Script

```
import argparse

from Crypto.Util.strxor import strxor
from pwn import context, serialtube

context.log_level = "error"

BUFFER_BLOCKS = 16
QUERY_TIMEOUT = 5

Z = b"\x00" * 16
def CST(k): return k.to_bytes(16, "big")

parser = argparse.ArgumentParser()
parser.add_argument("--port", default="/dev/ttyUSB0")
parser.add_argument("--baud", type=int, default=115386)
args = parser.parse_args()

io = serialtube(args.port, baudrate=args.baud, convert_newlines=False)
io.clean(0.3)

def recv_all(first_timeout=None, idle_timeout=1):
    data = b""
    while not data:
        data = io.recv(4096, timeout=first_timeout or 1)
        if first_timeout and not data:
            raise TimeoutError("no answer from the board")
    while True:
        chunk = io.recv(4096, timeout=idle_timeout)
        if not chunk:
            return data
        data += chunk

def encrypt(msg: bytes) -> bytes:
    assert len(msg) % 16 == 0 and len(msg) <= 16 * BUFFER_BLOCKS
    io.send(msg + Z * (BUFFER_BLOCKS - len(msg) // 16))
    out = io.recvn(16 * (BUFFER_BLOCKS + 1), timeout=QUERY_TIMEOUT)
    if len(out) != 16 * (BUFFER_BLOCKS + 1):
        raise TimeoutError(f"short answer from the board: {len(out)} bytes")
    return out

def blk(ct, i):  
    return ct[16*i:16*i+16]

print("Press the start button (B18) on the board to get the encrypted flag...")
flag_enc = recv_all()
assert len(flag_enc) % 16 == 0, f"unexpected flag length: {len(flag_enc)}"
print(f"encrypted flag: {flag_enc.hex()}")
ct = [flag_enc[i:i+16] for i in range(0, len(flag_enc), 16)]
nonce = ct[-1]
n = len(ct) - 1  

def getE(y: bytes) -> bytes:
    out = encrypt(CST(2) + Z + y)  
    return strxor(blk(out, 1), CST(3))

E0 = getE(Z)  

def getD(x: bytes) -> bytes:
    p1 = strxor(strxor(E0, CST(2)), x)
    out = encrypt(p1 + Z + Z)
    return strxor(strxor(blk(out, 1), E0), CST(3))

u = [None] * (n + 2)
u[n + 1] = nonce
for j in range(n - 1, -1, -1):
    u[j + 1] = getE(strxor(ct[j], u[j + 2]))

P = [nonce]
for k in range(1, n + 1):
    P.append(getD(strxor(strxor(u[k], P[k - 1]), CST(k))))

flag = b"".join(P[1:])
print(flag.decode(errors="replace"))

io.close()
```