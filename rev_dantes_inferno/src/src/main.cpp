#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_fixed.h"

#include "bn_sprite_ptr.h"
#include "bn_sprite_items_player.h"
#include "bn_sprite_items_beasts.h"
#include "bn_sprite_items_charon.h"
#include "bn_sprite_items_dis_gate.h"
#include "bn_sprite_items_ulysses_flame.h"
#include "bn_sprite_items_lucifer.h"

#include "bn_regular_bg_ptr.h"
#include "bn_regular_bg_items_arena.h"

#include "bn_sprite_text_generator.h"
#include "bn_sprite_font.h"
#include "bn_sprite_items_dialog_font.h"

#include "bn_vector.h"
#include "bn_string.h"
#include "bn_string_view.h"

#include "gerione.hpp"

#include <cstdint>
#include <cstddef>

namespace
{
    using u8 = std::uint8_t;
    using u32 = std::uint32_t;
    using u64 = std::uint64_t;

    enum class mode
    {
        roam,
        input,
        result,
        epilogue
    };

    struct chapter
    {
        const char* title;
        const char* actor;
        const char* line1;
        const char* line2;
        const char* prompt;
        bn::fixed npc_x;
        bn::fixed npc_y;
    };

    constexpr chapter chapters[] =
    {
        {
            "DARK WOOD",
            "Three beasts",
            "The straight path is lost.",
            "Three beasts bar the way.",
            "Demand a KEY",
            30, 6
        },
        {
            "CHARON",
            "Ferryman",
            "A coin outweighs fear.",
            //"Measure the two buffers.",
            "",
            "Numeric coin",
            42, 6
        },
        {
            "GATES OF DIS",
            "Sealed gates",
            "A host repeats lost echoes.",
            //"Match one decoded record.",
            "",
            "File contents",
            36, 4
        },
        {
            "MALEBOLGE",
            "Ulysses",
            "Not made to live as brutes.",
            //"Fill SBUCCIA's memory.",
            "",
            "Code for the VM",
            -30, 6
        },
        {
            "LUCIFER",
            "Virgil",
            "The world is upside down.",
            //"Encrypt Virgil's words.",
            "",
            "Hex",
            30, 4
        },
    };

    constexpr int chapter_count = int(sizeof(chapters) / sizeof(chapters[0]));

    constexpr char charset[] =
        " !\"#$%&'()*+,-./"
        "0123456789:;<=>?"
        "@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
        "`abcdefghijklmnopqrstuvwxyz{|}~";
    constexpr int charset_len = int(sizeof(charset)) - 1;

    struct alignas(4) answer_buffers
    {
        char dark_wood[24];
        char charon[24];
    };

    BN_DATA_EWRAM answer_buffers g_answer_buffers;

    constexpr u8 dark_wood_sbox[256] =
    {
        0x8C, 0x45, 0x1E, 0xD7, 0x90, 0x69, 0x22, 0xFB, 0xB4, 0x4D, 0x06, 0xDF, 0x98, 0x51, 0x2A, 0xE3,
        0xBC, 0x75, 0x0E, 0xC7, 0x80, 0x79, 0x32, 0xEB, 0xA4, 0x5D, 0x16, 0xCF, 0x88, 0x41, 0x3A, 0xF3,
        0xAC, 0x65, 0x3E, 0xF7, 0xB0, 0x49, 0x02, 0xDB, 0x94, 0x6D, 0x26, 0xFF, 0xB8, 0x71, 0x0A, 0xC3,
        0x9C, 0x55, 0x2E, 0xE7, 0xA0, 0x59, 0x12, 0xCB, 0x84, 0x7D, 0x36, 0xEF, 0xA8, 0x61, 0x1A, 0xD3,
        0xCC, 0x05, 0x5E, 0x97, 0xD0, 0x09, 0x42, 0x9B, 0xF4, 0x2D, 0x66, 0xBF, 0xF8, 0x31, 0x4A, 0x83,
        0xDC, 0x15, 0x70, 0xA7, 0xE0, 0x19, 0x52, 0x8B, 0xE4, 0x3D, 0x76, 0xAF, 0xE8, 0x21, 0x7A, 0xB3,
        0xEC, 0x25, 0x7E, 0xB7, 0xF0, 0x29, 0x62, 0xBB, 0xD4, 0x0D, 0x46, 0x9F, 0xD8, 0x11, 0x6A, 0xA3,
        0xFC, 0x35, 0x50, 0x87, 0xC0, 0x39, 0x72, 0xAB, 0xC4, 0x1D, 0x56, 0x8F, 0xC8, 0x01, 0x5A, 0x93,
        0x4C, 0x85, 0xDE, 0x17, 0x50, 0x89, 0xC2, 0x1B, 0x74, 0xAD, 0xE6, 0x3F, 0x78, 0xB1, 0xEA, 0x23,
        0x5C, 0x95, 0xCE, 0x07, 0x40, 0x99, 0xD2, 0x0B, 0x64, 0xBD, 0xF6, 0x2F, 0x68, 0xA1, 0xFA, 0x33,
        0x6C, 0xA5, 0xFE, 0x37, 0x70, 0xA9, 0xE2, 0x3B, 0x54, 0x8D, 0xC6, 0x1F, 0x58, 0x91, 0xCA, 0x03,
        0x7C, 0xB5, 0xEE, 0x27, 0x60, 0xB9, 0xF2, 0x2B, 0x44, 0x9D, 0xD6, 0x0F, 0x48, 0x81, 0xDA, 0x13,
        0x0C, 0xC5, 0x9E, 0x57, 0x10, 0xE9, 0xA2, 0x7B, 0x34, 0xCD, 0x86, 0x5F, 0x18, 0xD1, 0xAA, 0x63,
        0x1C, 0xD5, 0xAE, 0x67, 0x20, 0xF9, 0xB2, 0x6B, 0x24, 0xDD, 0x96, 0x4F, 0x08, 0xC1, 0xBA, 0x73,
        0x2C, 0xE5, 0xBE, 0x77, 0x30, 0xC9, 0x82, 0x5B, 0x14, 0xED, 0xA6, 0x7F, 0x38, 0xF1, 0x8A, 0x43,
        0x3C, 0xF5, 0x8E, 0x47, 0x00, 0xD9, 0x92, 0x4B, 0x04, 0xFD, 0xB6, 0x6F, 0x28, 0xE1, 0x9A, 0x53
    };

    constexpr u8 dark_wood_target[] =
    {
        0x9B, 0x70, 0x09, 0x3D, 0xF4, 0x83, 0x19, 0x4A, 0xD0
    };

    struct remote_file
    {
        const char* path;
        const u8* encoded_body;
        u8 length;
        u8 nonce;
    };

    constexpr u8 dis_body_0[] =
    {
        0xEB, 0xB5, 0x4A, 0x44, 0xF6, 0xA9, 0xCB, 0x0D,
        0xE3, 0x1B, 0xC1, 0x40, 0xAF, 0x54, 0x1E, 0x13
    };

    constexpr u8 dis_body_1[] =
    {
        0xC8, 0xD2, 0x84, 0xA7, 0x7A, 0x0D, 0x8A, 0x10,
        0xAF, 0x02, 0xA2
    };

    constexpr u8 dis_body_2[] =
    {
        0x3D, 0x49, 0xA6, 0x6C, 0x6E, 0xD8, 0x4E, 0x08,
        0xEC, 0x55, 0xE5, 0xB1, 0x5E, 0xBA, 0x64, 0x2A,
        0x3C, 0x76, 0x6D, 0xD4, 0xA3, 0x77, 0xC8, 0xBA,
        0x30, 0xD1, 0x5E, 0x56, 0x5C, 0x9F, 0xC7, 0xCA,
        0x1A, 0x81, 0x9A, 0xA4, 0x26, 0x14, 0xB4
    };

    constexpr u8 dis_body_3[] =
    {
        0xDF, 0x2D, 0x3F, 0x44, 0xDB, 0xA6, 0xB9, 0x08,
        0x4C, 0xE5, 0x03, 0xC5, 0xF9, 0x4F, 0x94, 0xD8,
        0x8C, 0xEB, 0x53, 0x94, 0xD6, 0x80, 0xB6, 0x46,
        0xE0
    };

    constexpr u8 dis_body_4[] =
    {
        0x40, 0x78, 0xE0, 0x3F, 0x56, 0xDB, 0xF5, 0x16,
        0x67, 0x37, 0xEF, 0x17, 0x45, 0x81, 0x9D, 0x91,
        0x13, 0x1F, 0xF1, 0x96
    };

    constexpr remote_file dis_files[] =
    {
        { "limbo/register.txt", dis_body_0, sizeof(dis_body_0), 0x13 },
        { "styx/mud.bin", dis_body_1, sizeof(dis_body_1), 0x37 },
        { "dis/messenger.sig", dis_body_2, sizeof(dis_body_2), 0x59 },
        { "dis/heresy.log", dis_body_3, sizeof(dis_body_3), 0x7B },
        { "dis/walls.txt", dis_body_4, sizeof(dis_body_4), 0x9D },
    };

    constexpr int dis_file_count = int(sizeof(dis_files) / sizeof(dis_files[0]));

    // The requested host fingerprint is independent of the embedded record table.
    const volatile u32 dis_target_masked[2] = { 0x0D5533A9u, 0xCE70F311u };

    constexpr u8 lucifer_key_masked[16] =
    {
        0x3F, 0x3B, 0xDB, 0xF2, 0xF8, 0x80, 0x86, 0xB0,
        0xBC, 0x49, 0x59, 0x05, 0x0B, 0x7A, 0x69, 0x5A
    };

    constexpr u8 lucifer_plain[8] = { 'S', 'E', 'E', '*', 'S', 'T', 'A', 'R' };

    constexpr u8 lucifer_sbox[16] =
    {
        0xC, 0x5, 0x6, 0xB, 0x9, 0x0, 0xA, 0xD,
        0x3, 0xE, 0xF, 0x8, 0x4, 0x7, 0x1, 0x2
    };

    bn::fixed clamp(bn::fixed v, bn::fixed lo, bn::fixed hi)
    {
        if(v < lo) return lo;
        if(v > hi) return hi;
        return v;
    }

    void clear_text(bn::vector<bn::sprite_ptr, 128>& text_sprites)
    {
        text_sprites.clear();
    }

    void draw_text(
        bn::sprite_text_generator& tg,
        bn::fixed x, bn::fixed y,
        const bn::string_view& str,
        bn::vector<bn::sprite_ptr, 128>& out)
    {
        tg.generate(x, y, str, out);
    }

    void draw_wrapped_text(
        bn::sprite_text_generator& tg,
        bn::fixed y,
        const bn::string_view& str,
        int max_chars,
        bn::vector<bn::sprite_ptr, 128>& out)
    {
        int start = 0;
        int line_count = 0;

        while(start < str.size() && line_count < 3)
        {
            while(start < str.size() && str[start] == ' ')
            {
                ++start;
            }

            if(start >= str.size())
            {
                break;
            }

            int end = start + max_chars;
            if(end >= str.size())
            {
                end = str.size();
            }
            else
            {
                int split = end;
                while(split > start && str[split] != ' ')
                {
                    --split;
                }

                if(split > start)
                {
                    end = split;
                }
            }

            bn::string<32> line;
            for(int index = start; index < end; ++index)
            {
                line.push_back(str[index]);
            }

            draw_text(tg, 0, y + line_count * 12, line, out);
            start = end;
            ++line_count;
        }
    }

    void copy_answer(char* dst, int dst_len, const bn::string<512>& src)
    {
        int i = 0;
        for(; i < dst_len - 1 && i < src.size(); ++i)
        {
            dst[i] = src[i];
        }
        dst[i] = 0;
    }

    bool check_dark_wood(const bn::string<512>& answer)
    {
        if(answer.size() != int(sizeof(dark_wood_target)))
        {
            return false;
        }

        u8 mismatch = 0;
        for(int i = 0; i < answer.size(); ++i)
        {
            const u8 ch = static_cast<u8>(answer[i]);
            mismatch |= u8(dark_wood_sbox[ch] ^ dark_wood_target[i]);
        }

        return mismatch == 0;
    }

    bool parse_decimal(const bn::string<512>& answer, int& value)
    {
        if(answer.empty())
        {
            return false;
        }

        if(answer.size() > 1 && answer[0] == '0')
        {
            return false;
        }

        value = 0;
        for(int i = 0; i < answer.size(); ++i)
        {
            const char ch = answer[i];
            if(ch < '0' || ch > '9')
            {
                return false;
            }

            const int digit = ch - '0';
            if(value > (0x7FFFFFFF - digit) / 10)
            {
                return false;
            }

            value = value * 10 + digit;
        }

        return true;
    }

    bool check_charon(const bn::string<512>& answer)
    {
        copy_answer(g_answer_buffers.charon, int(sizeof(g_answer_buffers.charon)), answer);

        const char* previous = &g_answer_buffers.dark_wood[0];
        const char* current = &g_answer_buffers.charon[0];
        int distance = current - previous;
        if(distance < 0)
        {
            distance = -distance;
        }

        const int mist = ((distance ^ 0x5A) + 0x31) ^ 0x31;
        const int unmasked = mist ^ 0x5A;

        int parsed = 0;
        return parse_decimal(answer, parsed) && parsed == unmasked;
    }

    u8 ror8(u8 value, int bits)
    {
        bits &= 7;
        return u8((value >> bits) | (value << ((8 - bits) & 7)));
    }

    u32 rol32(u32 value, int bits)
    {
        bits &= 31;
        if(! bits)
        {
            return value;
        }

        return (value << bits) | (value >> (32 - bits));
    }

    u64 rol64(u64 value, int bits)
    {
        bits &= 63;
        if(! bits)
        {
            return value;
        }

        return (value << bits) | (value >> (64 - bits));
    }

    u8 decode_remote_byte(const remote_file& file, int index)
    {
        const int rotation = ((index + file.nonce) % 7) + 1;
        const u8 mask = u8(0xA7 + file.nonce * 0x1D + index * 0x3B);
        return ror8(file.encoded_body[index], rotation) ^ mask;
    }

    u64 fingerprint_step(u64 fingerprint, u8 value, int index)
    {
        fingerprint = rol64(fingerprint ^ value, 11);
        fingerprint += u64(index + 1) * 0x9E3779B185EBCA87ull;
        fingerprint ^= fingerprint >> 17;
        return fingerprint;
    }

    u64 fingerprint_answer(const bn::string<512>& answer)
    {
        u64 fingerprint = 0x4449544553545958ull;

        for(int i = 0; i < answer.size(); ++i)
        {
            fingerprint = fingerprint_step(fingerprint, u8(answer[i]), i);
        }

        return fingerprint;
    }

    u64 fingerprint_file(const remote_file& file)
    {
        u64 fingerprint = 0x4449544553545958ull;

        for(int i = 0; i < file.length; ++i)
        {
            fingerprint = fingerprint_step(fingerprint, decode_remote_byte(file, i), i);
        }

        return fingerprint;
    }

    u64 dis_target_fingerprint()
    {
        const u32 low = dis_target_masked[0] ^ 0xA6D9345Cu;
        const u32 high = dis_target_masked[1] ^ 0x37C2E19Au;
        return (u64(high) << 32) | low;
    }

    u64 progress_step(u64 state, u8 value)
    {
        state ^= value;
        return state * 0x100000001B3ull;
    }

    void commit_progress(u64& state, int stage, const bn::string_view& answer)
    {
        state = progress_step(state, u8(0xA5u ^ u8(stage * 17)));
        state = progress_step(state, u8(answer.size()));

        for(int i = 0; i < answer.size(); ++i)
        {
            const u8 mask = u8(stage * 29 + i * 7);
            state = progress_step(state, u8(answer[i]) ^ mask);
        }

        state = progress_step(state, u8(0xFF - stage));
    }

    void commit_stage_progress(u64& state, int stage, const bn::string<512>& answer)
    {
        if(stage == 4)
        {
            bn::string<16> canonical;
            for(int i = 0; i < answer.size(); ++i)
            {
                char ch = answer[i];
                if(ch >= 'a' && ch <= 'f')
                {
                    ch = char(ch - ('a' - 'A'));
                }
                canonical.push_back(ch);
            }
            commit_progress(state, stage, canonical);
        }
        else
        {
            commit_progress(state, stage, answer);
        }
    }

    u64 finalize_progress(u64 state)
    {
        state ^= 0x00494E4645524E4Full;
        state ^= state >> 33;
        state *= 0xFF51AFD7ED558CCDull;
        state ^= state >> 33;
        state *= 0xC4CEB9FE1A85EC53ull;
        state ^= state >> 33;
        return state;
    }

    bool check_dis(const bn::string<512>& answer)
    {
        return fingerprint_answer(answer) == dis_target_fingerprint();
    }

    struct vm_ctx
    {
        const char* in = nullptr;
        int in_len = 0;
        int pos = 0;
        bn::string<256>* out = nullptr;
    };

    void vm_putc(int ch, void* user)
    {
        auto* ctx = static_cast<vm_ctx*>(user);
        if(ctx && ctx->out && ctx->out->size() < ctx->out->max_size())
        {
            ctx->out->push_back(char(ch & 0xFF));
        }
    }

    int vm_getc(void* user)
    {
        auto* ctx = static_cast<vm_ctx*>(user);
        if(ctx && ctx->in && ctx->pos < ctx->in_len)
        {
            return int(static_cast<u8>(ctx->in[ctx->pos++]));
        }

        return -1;
    }

    bool check_ulysses(const bn::string<512>& answer, bn::string<256>& vm_output)
    {
        vm_output.clear();

        vm_ctx ctx;
        ctx.in = answer.data();
        ctx.in_len = answer.size();
        ctx.out = &vm_output;

        const bool ok = dantes::romvm::run(
            dantes::romvm::challenge_image(),
            vm_putc,
            vm_getc,
            &ctx,
            100000
        );

        if(!ok && vm_output.empty())
        {
            vm_output = "SBUCCIA VM is not satisfied.";
        }

        return ok;
    }

    u32 load_be32(const u8* data)
    {
        return (u32(data[0]) << 24) | (u32(data[1]) << 16) | (u32(data[2]) << 8) | u32(data[3]);
    }

    u8 lucifer_key_byte(int index)
    {
        index &= 15;
        return lucifer_key_masked[index] ^ u8(0x6D + index * 17);
    }

    u32 load_key32(int offset)
    {
        return (u32(lucifer_key_byte(offset)) << 24) |
               (u32(lucifer_key_byte(offset + 1)) << 16) |
               (u32(lucifer_key_byte(offset + 2)) << 8) |
               u32(lucifer_key_byte(offset + 3));
    }

    u32 lucifer_f(u32 value, u32 key, int round)
    {
        u32 y = value ^ key ^ (u32(round) * 0x9E3779B9u);
        u32 z = 0;
        for(int nibble = 0; nibble < 8; ++nibble)
        {
            z |= u32(lucifer_sbox[(y >> (nibble * 4)) & 0xFu]) << (nibble * 4);
        }

        return rol32(z, (round % 7) + 3) + (key << 1) + 0x6C756366u;
    }

    void lucifer_decrypt_halves(u32& left, u32& right)
    {
        for(int round = 15; round >= 0; --round)
        {
            const u32 k0 = load_key32(round);
            const u32 k1 = load_key32(round + 4);
            const u32 round_key = k0 ^ rol32(k1, round % 13);
            const u32 previous_right = left;
            const u32 previous_left = right ^ lucifer_f(left, round_key, round);

            left = previous_left;
            right = previous_right;
        }
    }

    int from_hex(char ch)
    {
        if(ch >= '0' && ch <= '9') return ch - '0';
        if(ch >= 'a' && ch <= 'f') return 10 + ch - 'a';
        if(ch >= 'A' && ch <= 'F') return 10 + ch - 'A';
        return -1;
    }

    bool parse_hex_block(const bn::string<512>& answer, u32& left, u32& right)
    {
        if(answer.size() != 16)
        {
            return false;
        }

        left = 0;
        right = 0;
        for(int i = 0; i < 16; ++i)
        {
            const int nibble = from_hex(answer[i]);
            if(nibble < 0)
            {
                return false;
            }

            if(i < 8)
            {
                left = (left << 4) | u32(nibble);
            }
            else
            {
                right = (right << 4) | u32(nibble);
            }
        }

        return true;
    }

    bool check_lucifer(const bn::string<512>& answer)
    {
        u32 provided_left;
        u32 provided_right;
        if(! parse_hex_block(answer, provided_left, provided_right))
        {
            return false;
        }

        lucifer_decrypt_halves(provided_left, provided_right);

        const u32 plain_left = load_be32(lucifer_plain);
        const u32 plain_right = load_be32(lucifer_plain + 4);
        return ((provided_left ^ plain_left) | (provided_right ^ plain_right)) == 0;
    }

    void append_hex32(bn::string<64>& out, u32 value)
    {
        constexpr char digits[] = "0123456789ABCDEF";
        for(int shift = 28; shift >= 0; shift -= 4)
        {
            out.push_back(digits[(value >> shift) & 0xFu]);
        }
    }

    void append_hex64(bn::string<64>& out, u64 value)
    {
        append_hex32(out, u32(value >> 32));
        append_hex32(out, u32(value));
    }

    void append_recent_input(const bn::string<512>& typed, bn::string<64>& line)
    {
        line.append(">");
        int start = 0;
        if(typed.size() > 24)
        {
            start = typed.size() - 24;
        }

        for(int i = start; i < typed.size() && line.size() < line.max_size(); ++i)
        {
            line.push_back(typed[i]);
        }
    }

    void append_vm_output(const bn::string<256>& vm_output, bn::string<80>& out_line)
    {
        for(int i = 0; i < vm_output.size() && out_line.size() < out_line.max_size(); ++i)
        {
            char ch = vm_output[i];
            if(ch < 32 || ch > 126)
            {
                ch = '.';
            }
            out_line.push_back(ch);
        }
    }

    void prepare_input(int stage, bn::string<512>& typed)
    {
        typed.clear();

#if defined(DANTES_AUTOFILL_ANSWERS)
        constexpr const char* answers[] =
        {
            "GREYHOUND",
            "24",
            "HeavenlyMessenger:OPEN_THE_GATES_OF_DIS",
            "D4mNAt10",
            "FCE3DEC2506B2F99",
        };

        static_assert(int(sizeof(answers) / sizeof(answers[0])) == chapter_count);
        if(stage >= 0 && stage < chapter_count)
        {
            typed.append(answers[stage]);
        }
#else
        (void) stage;
#endif
    }

    bool check_answer(int stage, const bn::string<512>& answer, bn::string<256>& vm_output)
    {
        switch(stage)
        {
            case 0:
                if(check_dark_wood(answer))
                {
                    copy_answer(g_answer_buffers.dark_wood, int(sizeof(g_answer_buffers.dark_wood)), answer);
                    return true;
                }
                return false;

            case 1:
                return check_charon(answer);

            case 2:
                return check_dis(answer);

            case 3:
                return check_ulysses(answer, vm_output);

            case 4:
                return check_lucifer(answer);

            default:
                return false;
        }
    }

    bn::sprite_ptr create_chapter_sprite(int stage, bn::fixed x, bn::fixed y)
    {
        switch(stage)
        {
            case 0:
                return bn::sprite_items::beasts.create_sprite(x, y);
            case 1:
                return bn::sprite_items::charon.create_sprite(x, y);
            case 2:
                return bn::sprite_items::dis_gate.create_sprite(x, y);
            case 3:
                return bn::sprite_items::ulysses_flame.create_sprite(x, y);
            default:
                return bn::sprite_items::lucifer.create_sprite(x, y);
        }
    }
}

int main()
{
    bn::core::init();

    bn::regular_bg_ptr bg = bn::regular_bg_items::arena.create_bg(0, 0);

    bn::sprite_ptr player = bn::sprite_items::player.create_sprite(-92, 38);
    bn::sprite_ptr scene_sprite = create_chapter_sprite(0, chapters[0].npc_x, chapters[0].npc_y);

    bn::sprite_font font(bn::sprite_items::dialog_font);
    bn::sprite_text_generator text_gen(font);
    text_gen.set_center_alignment();

    bn::vector<bn::sprite_ptr, 128> ui_sprites;

    mode current_mode = mode::roam;
    int stage = 0;
    int cursor = 0;
    int result_timer = 0;
    int host_tick = 0;
    bool accepted = false;
    u64 progress = 0xCBF29CE484222325ull;

    bn::string<512> typed;
    bn::string<256> vm_output;

    const bn::fixed speed = 1;
    const bn::fixed half_w = 8;
    const bn::fixed half_h = 8;
    const bn::fixed min_x = -120 + half_w;
    const bn::fixed max_x =  120 - half_w;
    const bn::fixed min_y =  -80 + half_h;
    const bn::fixed max_y =   80 - half_h;

    while(true)
    {
        clear_text(ui_sprites);

        if(current_mode == mode::epilogue)
        {
            player.set_visible(true);
            player.set_y(34);
            scene_sprite.set_visible(false);

            draw_text(text_gen, 0, -52, "And thence we came forth", ui_sprites);
            draw_text(text_gen, 0, -34, "to see again the stars.", ui_sprites);
            bn::string<64> platform_flag("srdnlen{");
            append_hex64(platform_flag, finalize_progress(progress));
            platform_flag.append("}");

            draw_text(text_gen, 0, 4, "PLATFORM FLAG", ui_sprites);
            draw_text(text_gen, 0, 22, platform_flag, ui_sprites);
            draw_text(text_gen, 0, 54, "START: restart", ui_sprites);

            if(bn::keypad::start_pressed())
            {
                stage = 0;
                current_mode = mode::roam;
                progress = 0xCBF29CE484222325ull;
                player.set_position(-92, 38);
                scene_sprite = create_chapter_sprite(0, chapters[0].npc_x, chapters[0].npc_y);
            }

            bn::core::update();
            continue;
        }

        const chapter& ch = chapters[stage];
        const bool show_scene = current_mode == mode::roam;
        player.set_visible(show_scene);
        scene_sprite.set_visible(show_scene);
        scene_sprite.set_position(ch.npc_x, ch.npc_y);

        if(current_mode == mode::roam)
        {
            bn::fixed x = player.x();
            bn::fixed y = player.y();

            if(bn::keypad::left_held())  x -= speed;
            if(bn::keypad::right_held()) x += speed;
            if(bn::keypad::up_held())    y -= speed;
            if(bn::keypad::down_held())  y += speed;

            x = clamp(x, min_x, max_x);
            y = clamp(y, min_y, max_y);
            player.set_position(x, y);

            bn::fixed dx = player.x() - ch.npc_x;
            bn::fixed dy = player.y() - ch.npc_y;
            bn::fixed dist2 = dx * dx + dy * dy;

            constexpr bn::fixed interact_radius = 26;
            constexpr bn::fixed interact_radius2 = interact_radius * interact_radius;

            draw_text(text_gen, 0, -72, ch.title, ui_sprites);
            draw_text(text_gen, 0, -56, ch.line1, ui_sprites);
            draw_text(text_gen, 0, -42, ch.line2, ui_sprites);

            if(stage == 2)
            {
                const int idx = (host_tick / 80) % dis_file_count;
                bn::string<64> fp_line;
                fp_line.append("host fp ");
                append_hex64(fp_line, fingerprint_file(dis_files[idx]));
                draw_text(text_gen, 0, 52, fp_line, ui_sprites);
                ++host_tick;
            }
            else if(stage == 4)
            {
                draw_text(text_gen, 0, 52, "Virgil: SEE*STAR", ui_sprites);
            }

            if(dist2 <= interact_radius2)
            {
                draw_text(text_gen, 0, 68, "A: enter key", ui_sprites);
                if(bn::keypad::a_pressed())
                {
                    current_mode = mode::input;
                    cursor = 0;
                    prepare_input(stage, typed);
                    vm_output.clear();
                }
            }
        }
        else if(current_mode == mode::input)
        {
            if(bn::keypad::left_pressed())
            {
                cursor = (cursor - 1 + charset_len) % charset_len;
            }
            if(bn::keypad::right_pressed())
            {
                cursor = (cursor + 1) % charset_len;
            }
            if(bn::keypad::a_pressed() && typed.size() < typed.max_size())
            {
                typed.push_back(charset[cursor]);
            }
            if(bn::keypad::b_pressed() && ! typed.empty())
            {
                typed.pop_back();
            }
            if(bn::keypad::select_pressed())
            {
                current_mode = mode::roam;
            }
            if(bn::keypad::start_pressed())
            {
                accepted = check_answer(stage, typed, vm_output);
                if(accepted)
                {
                    commit_stage_progress(progress, stage, typed);
                }
                current_mode = mode::result;
                result_timer = 0;
            }

            draw_text(text_gen, 0, -70, ch.actor, ui_sprites);
            draw_text(text_gen, 0, -54, ch.prompt, ui_sprites);

            bn::string<64> line;
            append_recent_input(typed, line);
            draw_text(text_gen, 0, -16, line, ui_sprites);

            bn::string<64> selector;
            selector.append("selected [");
            selector.push_back(charset[cursor]);
            selector.append("]");
            draw_text(text_gen, 0, 8, selector, ui_sprites);

            draw_text(text_gen, 0, 44, "L/R change  A add  B delete", ui_sprites);
            draw_text(text_gen, 0, 60, "START submit  SELECT exit", ui_sprites);
        }
        else
        {
            ++result_timer;

            if(accepted)
            {
                draw_text(text_gen, 0, -56, "The key is accepted.", ui_sprites);

                if(stage == 0)
                {
                    draw_text(text_gen, 0, -34, "Virgil joins Dante.", ui_sprites);
                }
                else if(stage == 3)
                {
                    bn::string<80> line;
                    append_vm_output(vm_output, line);
                    draw_wrapped_text(text_gen, -34, line, 26, ui_sprites);
                }
                else
                {
                    draw_text(text_gen, 0, -34, "Dante may descend.", ui_sprites);
                }

                draw_text(text_gen, 0, 48, "A: continue", ui_sprites);

                if(result_timer > 20 && bn::keypad::a_pressed())
                {
                    ++stage;
                    if(stage >= chapter_count)
                    {
                        current_mode = mode::epilogue;
                    }
                    else
                    {
                        current_mode = mode::roam;
                        player.set_position(-92, 38);
                        scene_sprite = create_chapter_sprite(stage, chapters[stage].npc_x, chapters[stage].npc_y);
                    }
                }
            }
            else
            {
                draw_text(text_gen, 0, -48, "The way remains closed.", ui_sprites);
                if(stage == 3 && ! vm_output.empty())
                {
                    bn::string<80> line;
                    append_vm_output(vm_output, line);
                    draw_wrapped_text(text_gen, -26, line, 26, ui_sprites);
                }
                draw_text(text_gen, 0, 48, "A: try again", ui_sprites);

                if(result_timer > 20 && bn::keypad::a_pressed())
                {
                    current_mode = mode::input;
                    vm_output.clear();
                }
            }
        }

        bn::core::update();
    }
}
