#pragma once
#include "blockio.h"

// ensure one byte alignment for structures below
#pragma pack (push, 1)

// datum header
struct DatumHeader
{
    uint64_t next; // next datum if linked
    uint32_t totalLength; // total length of datum
    uint32_t length; // length of this datum segment
};

// 128-byte datum
typedef struct Datum
{
    DatumHeader header;
    uint8_t data[128 - sizeof(DatumHeader)];
}* LPDATUM;

// data page
typedef struct DataPage
{
    Datum data[1];
}* LPDATAPAGE;

using LPCDATAPAGE = const DataPage*;

// restore default structure alignment
#pragma pack (pop)

// datum macros
#define DATUM_NEXT(p, d)            ((p)->data[d].header.next)
#define DATUM_TOTAL_LENGTH(p, d)    ((p)->data[d].header.totalLength)
#define DATUM_LENGTH(p, d)          ((p)->data[d].header.length)
#define DATUM_PTR(p, d)             ((p)->data[d].data)

auto constexpr DATUM_PER_PAGE = BlockIO::BLOCK_SIZE / sizeof(Datum);

template <typename V>
class repository
{
public:
    void close()
    {
        m_io.close();
    }

    void insert(const V& value, uint64_t& offset)
    {
        writeValue(reinterpret_cast<const uint8_t*>(value.data()), value.size(), value.size(), offset);
    }

    void open(const char* filename)
    {
        m_filename = filename;
        m_io.open(filename, std::ios::in | std::ios::out | std::ios::trunc);
        m_io.writeblock(m_pageno, m_page.data());
    }

    void readVal(uint64_t offset, V& value)
    {
        uint64_t pageno = offset / BlockIO::BLOCK_SIZE;
        uint64_t datum = (offset - pageno * BlockIO::BLOCK_SIZE) / sizeof(Datum);

        auto ppage = reinterpret_cast<LPDATAPAGE>(m_page.data());
        m_io.readblock(pageno, ppage);

        auto totalLen = DATUM_TOTAL_LENGTH(ppage, datum);

        std::vector<uint8_t> buffer(totalLen);

        auto i = 0;
        for (;;) {
            auto* ptr = DATUM_PTR(ppage, datum);
            auto length = static_cast<int>(DATUM_LENGTH(ppage, datum));

            while (length > 0) {
                buffer[i++] = *ptr++;
                length--;
            }

            if ((offset = DATUM_NEXT(ppage, datum)) == 0)
                break;

            pageno = offset / BlockIO::BLOCK_SIZE;
            datum = (offset - pageno * BlockIO::BLOCK_SIZE) / sizeof(Datum);
            m_io.readblock(pageno, ppage);
        }

        value.assign(buffer.begin(), buffer.end());
    }

    void unlink()
    {
        m_io.unlink();
    }

private:
    void newpage()
    {
        auto ppage = reinterpret_cast<LPDATAPAGE>(m_page.data());

        memset(ppage, 0, BlockIO::BLOCK_SIZE);
        m_io.writeblock(++m_pageno, ppage);
    }

    uint64_t datumoffset() const
    {
        return datumoffset(m_pageno, m_datum);
    }

    uint64_t datumoffset(uint64_t pageno, uint8_t datum) const
    {
        return (pageno * BlockIO::BLOCK_SIZE) + (datum * sizeof(Datum));
    }

    uint64_t nextdatumoffset() const
    {
        auto pageno = m_pageno;
        auto ddatum = m_datum;

        if ((ddatum = static_cast<uint8_t>((ddatum + 1) % DATUM_PER_PAGE)) == 0) {
            pageno++;
        }

        return datumoffset(pageno, ddatum);
    }

    void newdatum()
    {
        if ((m_datum = static_cast<uint8_t>((m_datum + 1) % DATUM_PER_PAGE)) == 0) {
            auto ppage = reinterpret_cast<LPCDATAPAGE>(m_page.data());
            m_io.writeblock(m_pageno, ppage);
            newpage();
        }
    }

    void writeValue(const uint8_t* buffer, uint32_t length, uint32_t total, uint64_t& offset)
    {
        offset = datumoffset();

        auto ppage = reinterpret_cast<LPDATAPAGE>(m_page.data());

        m_io.readblock(m_pageno, ppage);

        constexpr auto avail = static_cast<uint32_t>(sizeof(Datum::data));

        for (auto i = 0; length > 0; ++i) {
            if (i > 0) {
                DATUM_NEXT(ppage, m_datum) = nextdatumoffset();
                newdatum();
            }

            uint32_t written = 0, nlength = std::min(length, avail);
            auto* ptr = DATUM_PTR(ppage, m_datum);
            while (nlength > 0) {
                *ptr++ = *buffer++;
                nlength--;
                written++;
            }

            DATUM_TOTAL_LENGTH(ppage, m_datum) = total;
            DATUM_LENGTH(ppage, m_datum) = written;
            length -= written;
        }

        m_io.writeblock(m_pageno, ppage);

        newdatum();
    }

    BlockIO m_io; // block i/o
    uint64_t m_pageno = 0; // current data page while writing
    uint8_t m_datum = 0; // current datum on data page while writing
    BlockIO::Block m_page{}; // disk page
    std::string m_filename; // file name
};
