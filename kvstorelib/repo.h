#pragma once
#include "blockio.h"
#include "value.h"

class Repository
{
public:
    Repository() = default;

    void close();
    void insert(const IValue& value, uint64_t& offset);
    void open(LPCWSTR filename);
    void readVal(uint64_t offset, IValue& value);
    void unlink();

private:
    uint64_t datumoffset() const;
    uint64_t datumoffset(uint64_t pageno, uint8_t datum) const;
    uint64_t nextdatumoffset() const;
    void newdatum();
    void newpage();
    void writeValue(uint8_t* buffer, uint32_t length, uint32_t total, uint64_t& offset);

    BlockIO m_io; // block i/o
    uint64_t m_pageno = 0; // current data page while writing
    uint8_t m_datum = 0; // current datum on data page while writing
    BlockIO::Block m_page{}; // disk page
    std::wstring m_filename; // file name
};
