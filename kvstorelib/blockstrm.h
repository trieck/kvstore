#pragma once

class BlockStream
{
public:
    BlockStream() = default;
    ~BlockStream();

    uint64_t size();
    uint64_t tell();
    void close();
    void flush();
    void create(LPSTORAGE pStorage, LPCWSTR streamName, 
        DWORD mode = STGM_CREATE | STGM_READWRITE | STGM_SHARE_EXCLUSIVE);
    void open(LPSTORAGE pStorage, LPCWSTR streamName, 
        DWORD mode = STGM_READWRITE | STGM_SHARE_EXCLUSIVE);
    ULONG readblock(uint64_t blockno, void* pv);
    void resize(uint64_t blocks);
    ULONG writeblock(uint64_t blockno, const void* pv);
    uint64_t seekblock(uint64_t blockno);

    static constexpr auto BLOCK_SIZE = 4096UL;

    using Block = std::array<char, BLOCK_SIZE>;

private:
    CComPtr<IStream> m_pStream;
};
