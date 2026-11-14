#include <cstdint>
#include <iostream>
#include <string>

using u8  = std::uint8_t;
using u64 = std::uint64_t;

static inline u64 fnv1a64_step(u64 h, u8 b)
{
    h ^= static_cast<u64>(b);
    h *= 0x100000001B3ull;   // FNV-1a 64 prime
    return h;
}

static constexpr u64 FNV_OFFSET_BASIS_64 = 0xCBF29CE484222325ull;

u64 vm_hash(const std::string& input)
{
    u64 state = FNV_OFFSET_BASIS_64;
    u8 length = 0;

    for(unsigned char byte : input)
    {
        if(byte == 0x0A || byte == 0x0D)
            break;

        state = fnv1a64_step(state, static_cast<u8>(byte));
        ++length;
    }

    return fnv1a64_step(state, length);
}

int main(int argc, char** argv)
{
    if(argc != 2)
    {
        std::cerr << "Usage: " << argv[0] << " \"string\"\n";
        return 1;
    }

    const u64 h = vm_hash(argv[1]);

    std::cout << std::hex << std::uppercase;
    std::cout << "H = 0x" << h << "\n";
    return 0;
}
