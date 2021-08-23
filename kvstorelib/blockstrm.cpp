#include "kvstorelib.h"
#include "blockstrm.h"

BlockStream::~BlockStream()
{
    close();
}

uint64_t BlockStream::size()
{
    if (!m_pStream) {
        return 0;
    }

    STATSTG stg;
    auto hr = m_pStream->Stat(&stg, STATFLAG_DEFAULT);
    _com_util::CheckError(hr);

    return stg.cbSize.QuadPart;
}

uint64_t BlockStream::tell()
{
    if (!m_pStream) {
        return 0;
    }

    LARGE_INTEGER move{};
    ULARGE_INTEGER result{};

    auto hr = m_pStream->Seek(move, STREAM_SEEK_CUR, &result);
    _com_util::CheckError(hr);

    return result.QuadPart;
}

void BlockStream::close()
{
    if (m_pStream != nullptr) {
        m_pStream.Release();
    }
}

void BlockStream::flush()
{
    if (m_pStream != nullptr) {
        auto hr = m_pStream->Commit(STGC_DEFAULT);
        _com_util::CheckError(hr);
    }
}

void BlockStream::create(LPSTORAGE pStorage, LPCWSTR streamName, DWORD mode)
{
    if (pStorage == nullptr || streamName == nullptr) {
        _com_issue_error(E_POINTER);
    }

    close();

    auto hr = pStorage->CreateStream(streamName, mode, 0, 0, &m_pStream);
    _com_util::CheckError(hr);
}

void BlockStream::open(LPSTORAGE pStorage, LPCWSTR streamName, DWORD mode)
{
    if (pStorage == nullptr || streamName == nullptr) {
        _com_issue_error(E_POINTER);
    }

    close();

    auto hr = pStorage->OpenStream(streamName, nullptr, mode, 0, &m_pStream);
    _com_util::CheckError(hr);
}

ULONG BlockStream::readblock(uint64_t blockno, void* pv)
{
    if (m_pStream == nullptr || pv == nullptr) {
        _com_issue_error(E_POINTER);
    }

    seekblock(blockno);

    ULONG read;
    auto hr = m_pStream->Read(pv, BLOCK_SIZE, &read);
    _com_util::CheckError(hr);

    return read;
}

uint64_t BlockStream::seekblock(uint64_t blockno)
{
    LARGE_INTEGER move;
    move.QuadPart = static_cast<int64_t>(blockno) * BLOCK_SIZE;

    ULARGE_INTEGER result{};

    auto hr = m_pStream->Seek(move, STREAM_SEEK_SET, &result);
    _com_util::CheckError(hr);

    return result.QuadPart;
}

ULONG BlockStream::writeblock(uint64_t blockno, const void* pv)
{
    if (m_pStream == nullptr || pv == nullptr) {
        _com_issue_error(E_POINTER);
    }

    seekblock(blockno);

    ULONG written;
    auto hr = m_pStream->Write(pv, BLOCK_SIZE, &written);
    _com_util::CheckError(hr);

    return written;
}

void BlockStream::resize(uint64_t blocks)
{
    if (m_pStream == nullptr) {
        _com_issue_error(E_POINTER);
    }

    ULARGE_INTEGER size;
    size.QuadPart = blocks * BLOCK_SIZE;

    auto hr = m_pStream->SetSize(size);
    _com_util::CheckError(hr);
}
