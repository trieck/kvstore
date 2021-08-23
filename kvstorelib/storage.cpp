#include "kvstorelib.h"
#include "storage.h"

/////////////////////////////////////////////////////////////////////////////
Storage::~Storage()
{
    close();
}

/////////////////////////////////////////////////////////////////////////////
void Storage::create(LPCWSTR fileName, DWORD mode)
{
    close();

    m_filename = fileName;

    std::filesystem::remove(fileName);

    STGOPTIONS options{};
    options.usVersion = STGOPTIONS_VERSION;
    options.ulSectorSize = 4096;

    auto hr = StgCreateStorageEx(fileName, mode, STGFMT_DOCFILE,
        0,
        &options,
        nullptr,
        IID_IStorage,
        reinterpret_cast<void**>(&m_pStorage));

    _com_util::CheckError(hr);
}

/////////////////////////////////////////////////////////////////////////////
void Storage::createStream(LPCWSTR name, IStream** ppStream, DWORD mode)
{
    if (m_pStorage == nullptr || ppStream == nullptr) {
        _com_issue_error(E_POINTER);
    }

    auto hr = m_pStorage->CreateStream(name, mode, 0, 0, ppStream);
    _com_util::CheckError(hr);
}

std::wstring Storage::filename() const
{
    return m_filename;
}

uint64_t Storage::size() const
{
    if (m_pStorage == nullptr) {
        _com_issue_error(E_POINTER);
    }

    STATSTG stg;
    auto hr = m_pStorage->Stat(&stg, STATFLAG_DEFAULT);
    _com_util::CheckError(hr);

    return stg.cbSize.QuadPart;
}

bool Storage::isOpen() const
{
    return m_pStorage != nullptr;
}

/////////////////////////////////////////////////////////////////////////////
void Storage::close()
{
    m_pStorage.Release();
}

/////////////////////////////////////////////////////////////////////////////
Storage::operator IStorage*()
{
    ATLASSERT(m_pStorage);

    return m_pStorage;
}
