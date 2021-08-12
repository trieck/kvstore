#include "kvstorelib.h"
#include "blockio.h"
#include "wexcept.h"

BlockIO::~BlockIO()
{
    close();
}

void BlockIO::open(LPCWSTR filename, std::ios::openmode mode)
{
    close();
    m_stream.open(filename, mode | std::ios::binary);
    if (!m_stream.is_open()) {
        auto message = boost::wformat(L"unable to open file \"%1%\".") % filename;
        throw wexception(message);
    }

    m_filename = filename;
}

void BlockIO::close()
{
    if (m_stream.is_open()) {
        m_stream.close();
    }
}

void BlockIO::unlink()
{
    close();
    //_unlink(filename_.c_str());
}

void BlockIO::readblock(uint64_t blockno, void* pv)
{
    seekblock(blockno);
    if (!m_stream.read(static_cast<char*>(pv), BLOCK_SIZE)) {
        throw std::runtime_error("cannot read block.");
    }
}

void BlockIO::writeblock(uint64_t blockno, const void* pv)
{
    seekblock(blockno);
    if (!m_stream.write(static_cast<const char*>(pv), BLOCK_SIZE)) {
        throw std::runtime_error("cannot write block.");
    }
}

void BlockIO::seekblock(uint64_t blockno)
{
    std::ostream::off_type offset = static_cast<int64_t>(blockno) * BLOCK_SIZE;
    if (!m_stream.seekp(offset, std::ios::beg)) {
        throw std::runtime_error("cannot seek block.");
    }
}

uint64_t BlockIO::tell()
{
    return m_stream.tellp();
}

uint64_t BlockIO::fileSize()
{
    auto save = m_stream.tellp();
    m_stream.seekp(0, std::ios::end);

    auto end = m_stream.tellp();
    m_stream.seekp(save, std::ios::beg);

    return end;
}

void BlockIO::flush()
{
    m_stream.flush();
}

std::wstring BlockIO::filename() const
{
    return m_filename;
}
