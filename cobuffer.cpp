#include "common.h"
#include "utility.h"

coobject::coobject(CoType type, REFGUID guid)
{
    m_builder.Clear();

    auto sGUID = utf8str(GuidToString(guid));

    auto guid_offset = m_builder.CreateString(static_cast<LPCSTR>(sGUID));
    auto root = CreateFBCoObject(m_builder, type, guid_offset);

    FinishFBCoObjectBuffer(m_builder, root);
}

const FBCoObject* coobject::buffer() const
{
    return GetFBCoObject(m_builder.GetBufferPointer());
}
