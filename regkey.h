#pragma once

class RegKey : public CRegKey
{
public:
    LSTATUS EnumKey(DWORD index, CString& name);
    LSTATUS QueryStringValue(LPCTSTR pszValueName, CString& value);
};
