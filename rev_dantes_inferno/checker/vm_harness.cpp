#include "gerione.hpp"

#include <cstring>
#include <iostream>
#include <string>

namespace
{
    struct io_context
    {
        const char* input;
        std::size_t position = 0;
        std::string output;
    };

    int read_byte(void* user)
    {
        auto& context = *static_cast<io_context*>(user);
        const unsigned char byte = static_cast<unsigned char>(context.input[context.position]);
        if(! byte)
            return -1;

        ++context.position;
        return byte;
    }

    void write_byte(int byte, void* user)
    {
        auto& context = *static_cast<io_context*>(user);
        context.output.push_back(static_cast<char>(byte));
    }

    bool run_key(const char* key)
    {
        io_context context{ key };
        const bool accepted = dantes::romvm::run(
            dantes::romvm::challenge_image(),
            write_byte,
            read_byte,
            &context,
            100000);

        std::cout << key << ": " << (accepted ? "accepted" : "rejected");
        if(! context.output.empty())
            std::cout << " (" << context.output << ")";
        std::cout << '\n';
        return accepted;
    }
}

int main()
{
    const bool new_key = run_key("D4mNAt10");
    const bool old_key = run_key("W1H31l");
    const bool near_miss = run_key("D4mNAt11");
    return new_key && ! old_key && ! near_miss ? 0 : 1;
}
