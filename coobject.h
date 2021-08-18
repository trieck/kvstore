#pragma once

#include "coobject_generated.h"

/////////////////////////////////////////////////////////////////////////////
class coobject
{
public:
    coobject() = default;
    coobject(coobject&& rhs) noexcept;

    coobject& operator=(coobject&& rhs) noexcept;

    CoType type() const;
    std::string guid() const;


    void copy(const uint8_t* data, uint32_t size);
    uint8_t* data() const;
    uint32_t size() const;

    void assign(std::vector<uint8_t>::const_iterator begin, std::vector<uint8_t>::const_iterator end);

    bool operator ==(const coobject& rhs);

    template <typename T>
    const T* as() const;

    const CoObject& buffer() const;

protected:
    flatbuffers::FlatBufferBuilder m_builder;
};

template <typename T>
const T* coobject::as() const
{
    return buffer().type_as<T>();
}

/////////////////////////////////////////////////////////////////////////////
class coclass : public coobject
{
public:
    coclass() = delete;
    coclass(coobject&& rhs);
    coclass& operator=(coobject&& rhs);

    coclass(const std::wstring& clsID,
            const std::wstring& appID = L"",
            const wstring_set& catIDs = {});


    LPCSTR appID() const;
};

/////////////////////////////////////////////////////////////////////////////
class coapp : public coobject
{
public:
    coapp() = delete;
    coapp(coobject&& rhs);
    coapp& operator=(coobject&& rhs);

    coapp(const std::wstring& appID, const wstring_set& clsIDs = {});
};

/////////////////////////////////////////////////////////////////////////////
class cocat : public coobject
{
public:
    cocat() = delete;
    cocat(coobject&& rhs);
    cocat& operator=(coobject&& rhs);

    cocat(const std::wstring& catID, const wstring_set& clsIDs = {});
};
