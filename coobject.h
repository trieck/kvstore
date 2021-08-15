#pragma once

#include "coobject_generated.h"
#include "value.h"

/////////////////////////////////////////////////////////////////////////////
class coobject : public IValue
{
public:
    coobject() = default;

    CoType type() const;
    std::string guid() const;

    // IValue members
    void copy(const uint8_t* data, uint32_t size) override;
    uint8_t* data() const override;
    uint32_t size() const override;

    bool operator ==(const IValue& rhs);

    template <typename T>
    const T* as() const;

protected:
    const CoObject& buffer() const;
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
    coapp(const std::wstring& appID, const wstring_set& clsIDs = {});
};
