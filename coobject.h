#pragma once

#include "coobject_generated.h"

/////////////////////////////////////////////////////////////////////////////
class coclass : public flatbuffers::FlatBufferBuilder
{
public:
    coclass() = default;
    coclass(const std::wstring& clsID,
            const std::wstring& appID,
            const wstring_set& catIDs);
};

/////////////////////////////////////////////////////////////////////////////
class coapp : public flatbuffers::FlatBufferBuilder
{
public:
    coapp() = default;
    coapp(const std::wstring& appID, const wstring_set& clsIDs);
};
