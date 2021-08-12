#pragma once

class BlockIO
{
public:
    BlockIO() = default;
    ~BlockIO();

    std::wstring filename() const;
    uint64_t fileSize();
    uint64_t tell();
    void close();
    void flush();
    void open(LPCWSTR filename, std::ios::openmode mode);
    void readblock(uint64_t blockno, void* pv);
    void seekblock(uint64_t blockno);
    void unlink();
    void writeblock(uint64_t blockno, const void* pv);

    static constexpr auto BLOCK_SIZE = 4096UL;

    using Block = std::array<char, BLOCK_SIZE>;
private:
    std::wstring m_filename;
    std::fstream m_stream;
};
