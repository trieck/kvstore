#pragma once

#include "coobject_generated.h"

class coobject
{
public:
    coobject(CoType type, REFGUID guid);

    const FBCoObject* buffer() const;

private:
    flatbuffers::FlatBufferBuilder m_builder;
};
