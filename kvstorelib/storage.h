#pragma once

class Storage
{
public:
    Storage() = default;
    ~Storage();

    void create(LPCWSTR fileName, DWORD mode = STGM_CREATE
                       | STGM_READWRITE | STGM_SHARE_EXCLUSIVE);
    void createStream(LPCWSTR name, IStream** ppStream, DWORD mode = STGM_CREATE
                             | STGM_READWRITE | STGM_SHARE_EXCLUSIVE);

    std::wstring filename() const;

    uint64_t size() const;
    bool isOpen() const;
    void close();

    operator IStorage*();

private:
    std::wstring m_filename;
    CComPtr<IStorage> m_pStorage;
};
