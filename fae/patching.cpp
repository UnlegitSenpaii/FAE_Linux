#include <algorithm>
#include <iostream>
#include <fae/patching.hpp>

namespace fae
{
    Patch::Patch(int replace, std::string name, aob_t pattern)
        : m_name{name}
        , m_pattern{pattern}
        , m_replace{makeReplace(pattern, replace)}
    {
    }

    const std::string& Patch::name() const
    {
        return m_name;
    }

    std::uint32_t Patch::apply(aob_t& data) const
    {
        std::uint32_t count{0};
        aob_t::iterator found{std::begin(data)};

        while ((found = std::search(found, std::end(data), std::begin(m_pattern), std::end(m_pattern))) != std::end(data)) {
            std::copy(std::begin(m_replace), std::end(m_replace), found);
            found += m_replace.size();
            ++count;
        }

        return count;
    }

    aob_t Patch::makeReplace(const aob_t& data, int replace)
    {
        if (data.size() < 2)
            return {};

        aob_t replacePattern{};

        //do I get paid for writing this in a clean way? no. do I get paid at all? no. :(
        switch (replace) {
            case 0://JZ -> JNZ (0x75 or 0x85) instruction
                if (data[0] != 0x74 && data[0] != 0x84) //Check for JZ (0x74 or 0x84)
                    return {};
                replacePattern.push_back(data[0] == 0x74 ? 0x75 : 0x85);
                break;

            case 1://JZ -> JMP (0xEB) instruction
                if (data[0] != 0x74 && data[0] != 0x84) //Check for JZ
                    return {};
                replacePattern.push_back(0xEB);
                break;

            case 2:// JNZ (0F 85)-> JMP ~ huh? im surprised this doesn't break stuff -- it did indeed break stuff
                if (data[0] != 0x0F && data[1] != 0x85 && // Check for JNZ (0F 85)
                    data[0] != 0x75)                               // Check for JNZ SHORT (75)
                    return {};
                replacePattern.push_back(0xEB);
                break;

            case 3://CMOVNZ (0F 45) -> CMOVZ (0F 44) -- Prefix (0f) discarded!!
                if (data[0] != 0x45) //Check for CMOVNZ (0F 45) -- Prefix (0f) discarded!!
                    return {};
                replacePattern.push_back(0x44);
                break;

            default:
                std::cerr << "[FAE]" << "Error: Patch Instruction Mode '" << replace << "' not implemented yet!" << std::endl;
                return {};
        }

        replacePattern.insert(replacePattern.end(), data.begin() + replacePattern.size(), data.end());
        return replacePattern;
    }
}
