#pragma once

class BlockIO
{
public:
    BlockIO() = default;
    ~BlockIO();

    std::string filename() const;
    uint64_t fileSize();
    uint64_t tell();
    void close();
    void flush();
    void open(const char* filename, std::ios::openmode mode);
    void readblock(uint64_t blockno, void* pv);
    void seekblock(uint64_t blockno);
    void unlink();
    void writeblock(uint64_t blockno, const void* pv);
    void resize(uint64_t blocks);

    static constexpr auto BLOCK_SIZE = 4096UL;

    using Block = std::array<char, BLOCK_SIZE>;
private:
    std::string m_filename;
    std::fstream m_stream;
};
