#pragma once

class RandomPerm
{
public:
    void generate(uint64_t n);

    uint64_t size() const
    {
        return m_table.size();
    }

    uint64_t operator[](uint64_t index) const;

private:
    uint64_t uniform(uint64_t i);
    std::mt19937 m_generator;
    std::vector<uint64_t> m_table;
};
