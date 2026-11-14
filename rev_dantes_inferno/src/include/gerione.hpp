#pragma once
#include <cstdint>
#include <cstddef>
#include "bn_common.h"

namespace dantes::romvm
{
    using u8  = std::uint8_t;
    using u16 = std::uint16_t;
    using u32 = std::uint32_t;

    using PutChar = void(*)(int ch, void* user);   // 0..255
    using GetChar = int(*)(void* user);            // 0..255 or -1

    // payload = code_raw[code_len] || mem_init[mem_len]
    struct __attribute__((packed, aligned(4))) VmImage
    {
        u32 magic;        // 'DVM1'
        u32 code_len;
        u32 mem_len;
        u32 seed;         // low 8 bits used
        u32 reserved_lo;  // expected hash low 32
        u32 reserved_hi;  // expected hash high 32
        u32 guard_meta;   // masked input length and guard signature
        u8  guard[12];    // masked partial input constraints, checksum, and decoys
        u8  payload[1];
    };

    static constexpr std::size_t kCodeMax = 256;
    static constexpr std::size_t kMemSize = 256;

    BN_DATA_EWRAM static u8 g_code[kCodeMax];
    BN_DATA_EWRAM static u8 g_mem[kMemSize];

    static constexpr u8 perm16[16] =
    {
        0x0, 0x9, 0x2, 0xB,
        0x4, 0xD, 0x6, 0xF,
        0x8, 0x1, 0xA, 0x3,
        0xC, 0x5, 0xE, 0x7
    };

    enum Op : u8
    {
        OP_NOP = 0,
        OP_ADP = 1,   // dp += arg
        OP_LDA = 2,   // A = mem[dp]
        OP_STA = 3,   // mem[dp] = A
        OP_XOR = 4,   // A ^= mem[dp]
        OP_ADD = 5,   // A += mem[dp]
        OP_ROL = 6,   // A = rol8(A, arg?arg:1)
        OP_CRZ = 7,   // A = crz(A, mem[dp])
        OP_IN = 8,   // A = getc() (EOF->0)
        OP_OUT = 9,   // putc(A)
        OP_MIX = 10,  // hash lane
        OP_JZ = 11,  // if A==0 -> ip = next_u16()
        OP_JMP = 12,  // ip = next_u16()
        OP_HALT = 13,  // halt
        OP_SDP8 = 14,  // dp = next_u8()
        OP_BZ = 15
    };

    struct VM
    {
        u16 ip = 0;
        u8 dp = 0;     // "byte index" for hashing
        u8 A = 0;
        std::uint64_t B = 0;
        u8 seed = 0;
        u8 input_len = 0;
        bool input_overflow = false;
        u16 code_len = 0;
        bool halted = false;
        PutChar putc = nullptr;
        GetChar getc = nullptr;
        void* user = nullptr;
    };

    static inline u8 rol8(u8 x, unsigned r)
    {
        r &= 7u;
        return static_cast<u8>((x << r) | (x >> ((8u - r) & 7u)));
    }

    using u64 = std::uint64_t;

    static inline u64 fnv1a64_step(u64 h, u8 b)
    {
        // FNV-1a 64-bit
        h ^= static_cast<u64>(b);
        h *= 0x100000001B3ull; // FNV prime
        return h;
    }

    static inline u8 sbox(u8 x) // bijection mod 256
    {
        return static_cast<u8>(x * 29u + 37u);
    }

    static inline u8 crz_byte(u8 a, u8 b)
    {
        static constexpr u8 MAP[3][3] =
        {
            { 1, 0, 0 },
            { 1, 0, 2 },
            { 2, 2, 1 },
        };

        u8 out = 0, pow = 1;
        for(int i = 0; i < 6; ++i)
        {
            const u8 ta = static_cast<u8>(a % 3u);
            const u8 tb = static_cast<u8>(b % 3u);
            a = static_cast<u8>(a / 3u);
            b = static_cast<u8>(b / 3u);

            out = static_cast<u8>(out + MAP[ta][tb] * pow);
            pow = static_cast<u8>(pow * 3u);
        }
        return out;
    }

    static inline u8 fetch_decoded(VM& v)
    {
        if(v.ip >= v.code_len || v.ip >= kCodeMax)
        {
            v.halted = true;
            return 0;
        }

        const u16 pos = v.ip;
        const u8  raw = g_code[pos];
        
        const u8 key = static_cast<u8>((pos * 13u + v.seed + 0x5Au) & 0xFFu);
        const u8 dec = static_cast<u8>(raw ^ key);

        v.ip = static_cast<u16>(pos + 1u);
        return dec;
    }

    static inline u16 fetch_u16(VM& v)
    {
        const u16 lo = fetch_decoded(v);
        const u16 hi = fetch_decoded(v);
        return static_cast<u16>(lo | (hi << 8));
    }

    static inline void step(VM& v)
    {
        if(v.halted)
            return;

        const u8 dec = fetch_decoded(v);
        const u8 op  = perm16[dec & 0x0Fu];
        const u8 arg = static_cast<u8>(dec >> 4);

        switch(op)
        {
            case OP_NOP: break;

            case OP_ADP: v.dp = static_cast<u8>(v.dp + arg); break;

            case OP_LDA: v.A = g_mem[v.dp]; break;
            case OP_STA: g_mem[v.dp] = v.A; break;

            case OP_XOR: v.A = static_cast<u8>(v.A ^ g_mem[v.dp]); break;
            case OP_ADD: v.A = static_cast<u8>(v.A + g_mem[v.dp]); break;
            case OP_ROL: v.A = rol8(v.A, arg ? arg : 1u); break;

            case OP_CRZ: v.A = crz_byte(v.A, g_mem[v.dp]); break;

            case OP_MIX:
            {
                if(v.B == 0ull) v.B = 0xCBF29CE484222325ull;
                v.B = fnv1a64_step(v.B, v.A);
                ++v.dp;
                break;
            }

            case OP_IN:
            {
                int x = -1;
                if(v.getc) x = v.getc(v.user);

                if(x < 0) v.A = 0u;
                else
                {
                    const u8 ch = static_cast<u8>(x & 0xFF);
                    v.A = ch;

                    if(ch != 0x0Au && ch != 0x0Du)
                    {
                        if(v.input_len < 16u)
                        {
                            const u8 slot = static_cast<u8>((v.input_len * 5u + 3u) & 0x0Fu);
                            const u8 mask = rol8(
                                static_cast<u8>(v.seed + 0x6Du + v.input_len * 19u),
                                (v.input_len % 7u) + 1u);
                            g_mem[0xF0u + slot] = static_cast<u8>(ch ^ mask);
                            ++v.input_len;
                        }
                        else
                        {
                            v.input_overflow = true;
                        }
                    }
                }
                break;
            }

            case OP_OUT:
                if(v.putc) v.putc(static_cast<int>(v.A), v.user);
                break;

            case OP_JZ:
            {
                const u16 tgt = fetch_u16(v);
                if(v.A == 0u)
                {
                    if(tgt < v.code_len) v.ip = tgt;
                    else v.halted = true;
                }
                break;
            }

            case OP_JMP:
            {
                const u16 tgt = fetch_u16(v);
                if(tgt < v.code_len) v.ip = tgt;
                else v.halted = true;
                break;
            }

            case OP_SDP8:
                v.dp = fetch_decoded(v);
                break;

            case OP_BZ:
                v.A = (v.B == 0u) ? 0u : 1u;
                break;

            case OP_HALT:
            default:
                v.halted = true;
                break;
        }
    }

    static inline u8 sealed_input_byte(const VM& v, u8 index)
    {
        const u8 slot = static_cast<u8>((index * 5u + 3u) & 0x0Fu);
        const u8 mask = rol8(
            static_cast<u8>(v.seed + 0x6Du + index * 19u),
            (index % 7u) + 1u);
        return static_cast<u8>(g_mem[0xF0u + slot] ^ mask);
    }

    static inline bool verify_memory_seal(const VmImage* img, const VM& v)
    {
        if((img->guard_meta & 0xFFFFFF00u) != 0x53425500u || v.input_overflow)
            return false;

        const u8 required_len = static_cast<u8>((img->guard_meta ^ 0x534255A7u) & 0xFFu);
        if(required_len == 0u || required_len > 10u || v.input_len != required_len)
            return false;

        u8 checksum = 0x42u;
        u8 mismatch = 0;
        for(u8 i = 0; i < required_len; ++i)
        {
            const u8 current = sealed_input_byte(v, i);
            const u8 shard = rol8(
                static_cast<u8>(
                    sbox(static_cast<u8>(current ^ static_cast<u8>(0x31u + i * 11u))) +
                    static_cast<u8>(0x5Cu + i * 17u)),
                (i % 7u) + 1u);
            const u8 slot = static_cast<u8>((i * 3u + 2u) % required_len);
            const u8 expected = static_cast<u8>(img->guard[slot] ^ static_cast<u8>(0xA5u + slot * 13u));
            const u8 bit_mask = rol8(0x1Fu, i * 3u + 1u);

            mismatch |= static_cast<u8>((shard ^ expected) & bit_mask);

            checksum = static_cast<u8>(rol8(static_cast<u8>(checksum ^ current), 1u) + i * 9u);
        }

        mismatch |= static_cast<u8>(checksum ^ static_cast<u8>(img->guard[required_len] ^ 0x6Du));
        return mismatch == 0u;
    }

    static inline bool run(const VmImage* img, PutChar putc, GetChar getc, void* user, int max_steps = 50000)
    {
        if(!img || img->magic != 0x314D5644u)
            return false;

        if(img->code_len > kCodeMax || img->mem_len > kMemSize)
            return false;

        const u8* code_raw = img->payload;
        const u8* mem_init = img->payload + img->code_len;

        for(std::size_t i = 0; i < kCodeMax; ++i) g_code[i] = 0;
        for(std::size_t i = 0; i < kMemSize; ++i) g_mem[i] = 0;

        for(u32 i = 0; i < img->code_len; ++i) g_code[i] = code_raw[i];
        for(u32 i = 0; i < img->mem_len; ++i) g_mem[i] = mem_init[i];

        VM v;
        v.seed = static_cast<u8>(img->seed & 0xFFu);
        v.code_len = static_cast<u16>(img->code_len);
        v.putc = putc;
        v.getc = getc;
        v.user = user;
        v.dp = 0; // reset hash index/length

        for(int i = 0; i < max_steps && !v.halted; ++i)
            step(v);

        // Finalize only after both the bytecode and the scattered memory seal agree.
        u64 h = v.B;
        if(h == 0ull) h = 0xCBF29CE484222325ull;

        // Absorb the number of mixed input bytes.
        h = fnv1a64_step(h, v.dp);

        // expected 64-bit from header
        const u64 expected = (static_cast<u64>(img->reserved_hi) << 32) |
                            static_cast<u64>(img->reserved_lo);

        bool ok = v.halted && verify_memory_seal(img, v) && (h == expected);

        /*
        if(putc)
        {
            auto hex64 = [](u64 x, PutChar p, void* u){
                const char* d = "0123456789ABCDEF";
                for(int i = 15; i >= 0; --i)
                    p(d[(x >> (i * 4)) & 0xFULL], u);
            };

            putc('H', user); putc('=', user); hex64(h, putc, user);
            putc(' ', user);
            putc('E', user); putc('=', user); hex64(expected, putc, user);
        }
        */
        
        if(ok && putc)
        {
            const char* msg = "SBUCCIA is satisfied.";
            for(const char* p = msg; *p; ++p)
                putc(*p, user);
        }

        return ok;
    }

    alignas(4) const unsigned char MB_IMAGE_CHALLENGE[] = {
        0x44, 0x56, 0x4D, 0x31, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0xAA, 0x7E, 0x07, 0x28, 0x0B, 0x95, 0x18, 0xAA,
        0xAF, 0x55, 0x42, 0x53, 0x8E, 0x32, 0x3A, 0xC2, 0x69, 0x0F, 0xCD, 0x9D,
        0x87, 0x39, 0xC7, 0x51,
        0x52, 0x64, 0x7C, 0x81, 0x84, 0x97, 0xA8, 0xB5, 0xC7,
    };

    static inline const VmImage* challenge_image()
    {
        return reinterpret_cast<const VmImage*>(MB_IMAGE_CHALLENGE);
    }
}
