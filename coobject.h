#pragma once

#include "coobject_generated.h"

class coobject : public flatbuffers::FlatBufferBuilder
{
public:
    coobject() = default;
    coobject(CoType type, REFGUID guid);

    const FBCoObject* buffer() const;
};
