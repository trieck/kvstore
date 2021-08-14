
#include "common.h"
#include "utility.h"

std::wstring GuidToString(REFGUID guid)
{
    wchar_t buff[40] = {};

    (void)StringFromGUID2(guid, buff, 40);

    return buff;
}

HRESULT StringToGUID(LPCOLESTR str, GUID& guid)
{
    auto hr = CLSIDFromString(str, &guid);
    return hr;
}
