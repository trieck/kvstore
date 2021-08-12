#pragma once
#include "blockio.h"

class Repository
{
public:
    Repository() = default;

    void open(LPCWSTR filename);
    void close();
    void unlink();

private:
    BlockIO m_io; // block i/o
    uint64_t m_pageno = 0; // current data page while writing
    uint8_t m_ddatum = 0; // current datum on data page while writing
    BlockIO::Block m_page{}; // disk page
};
