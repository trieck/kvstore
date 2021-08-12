
#include "common.h"
#include "utility.h"

std::wstring GuidToString(REFGUID guid)
{
    wchar_t buff[40] = {};

    StringFromGUID2(guid, buff, 40);

    return buff;
}
