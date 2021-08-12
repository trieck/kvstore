#include "kvstorelib.h"
#include "randperm.h"

void RandomPerm::generate(uint64_t n)
{
    m_table.clear();
    m_table.resize(n);

    std::random_device rd;
    m_generator.seed(rd());

    for (uint64_t i = 0, j; i < n; ++i) {
        j = uniform(i);
        m_table[i] = m_table[j];
        m_table[j] = i;
    }
}

uint64_t RandomPerm::operator[](uint64_t index) const
{
    if (index >= m_table.size())
        throw std::out_of_range("permutation index out of range.");

    return m_table[index];
}

uint64_t RandomPerm::uniform(uint64_t i)
{
    std::uniform_int_distribution<uint64_t> dis(0, i);
    return dis(m_generator);
}
