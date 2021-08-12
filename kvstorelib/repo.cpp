#include "kvstorelib.h"
#include "repo.h"

// ensure one byte alignment for structures below
#pragma pack (push, 1)

// datum header
struct DatumHeader
{
    uint64_t next; // next datum if linked
    uint32_t totalLength; // total length of datum
    uint32_t length; // length of this datum segment
};

// 512-byte datum
typedef struct Datum
{
    DatumHeader header;
    uint8_t data[512 - sizeof(DatumHeader)];
}* LPDATUM;

// data page
typedef struct DataPage
{
    Datum data[1];
}* LPDATAPAGE;

// restore default structure alignment
#pragma pack (pop)

auto constexpr DATUM_PER_PAGE = BlockIO::BLOCK_SIZE / sizeof(Datum);

void Repository::open(LPCWSTR filename)
{
    m_io.open(filename, std::ios::in | std::ios::out | std::ios::trunc);
    m_io.writeblock(m_pageno, m_page.data());
}

void Repository::close()
{
}

void Repository::unlink()
{
}
