#include "common.h"
#include "utility.h"

coobject::coobject(CoType type, REFGUID guid)
{
    auto sGUID = utf8str(GuidToString(guid));

    auto guid_offset = CreateString(static_cast<LPCSTR>(sGUID));
    auto root = CreateFBCoObject(*this, type, guid_offset);

    FinishFBCoObjectBuffer(*this, root);
}

const FBCoObject* coobject::buffer() const
{
    return GetFBCoObject(GetBufferPointer());
}

