#include "common.h"
#include "regkey.h"

LSTATUS RegKey::QueryStringValue(LPCTSTR pszValueName, CString& value)
{
    ULONG uLength = 0;
    auto result = CRegKey::QueryStringValue(pszValueName, nullptr, &uLength);
    if (result != ERROR_SUCCESS) {
        return result;
    }

    value.Empty();
    auto* pBuffer = value.GetBuffer(static_cast<int>(uLength));

    result = CRegKey::QueryStringValue(pszValueName, pBuffer, &uLength);

    value.ReleaseBuffer();

    return result;
}

LSTATUS RegKey::EnumKey(DWORD index, CString& name)
{
    DWORD dwLength = 1024;

    name.Empty();
    auto* pBuffer = name.GetBuffer(static_cast<int>(dwLength));
    
    auto result = CRegKey::EnumKey(index, pBuffer, &dwLength);
    
    name.ReleaseBuffer();

    return result;
}
