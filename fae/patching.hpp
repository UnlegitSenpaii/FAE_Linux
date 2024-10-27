#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fae
{
    using aob_t = std::vector<std::uint8_t>;

    class Patch final
    {
    public:
        Patch(int replace, std::string name, aob_t pattern);

        Patch(const Patch&) = default;
        Patch& operator=(const Patch&) = default;

        Patch(Patch&&) = default;
        Patch& operator=(Patch&&) = default;

        ~Patch() = default;

        const std::string& name() const;

        std::uint32_t apply(aob_t& data) const;

        static aob_t makeReplace(const aob_t& data, int replace);

    private:
        std::string m_name;
        aob_t m_pattern;
        aob_t m_replace;
    };

} // Patcher
