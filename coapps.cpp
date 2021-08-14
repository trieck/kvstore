#include "common.h"
#include "coapps.h"
#include "regkey.h"

bool CoApps::Construct()
{
    RegKey key, subKey;

    auto result = key.Open(HKEY_CLASSES_ROOT, L"AppID", KEY_READ);
    if (result != ERROR_SUCCESS) {
        return false;
    }

    DWORD index = 0;
    CString appID;

    for (;;) {
        result = key.EnumKey(index++, appID);
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (result != ERROR_SUCCESS) {
            continue;
        }

        result = subKey.Open(key.m_hKey, appID, KEY_READ);
        if (result != ERROR_SUCCESS) {
            continue;
        }

        if (appID[0] != '{') {
            subKey.QueryStringValue(_T("AppID"), appID);
        }

        if (appID[0] == _T('\0')) {
            continue; // not much chance of success
        }
        
        m_appids.insert(static_cast<LPCWSTR>(appID));
    }

    return true;
}

void CoApps::addClass(LPCWSTR appID, LPCWSTR clsID)
{
    m_appids.insert(appID);

    auto& it = m_appClsids[appID];
    it.insert(clsID);
}

void CoApps::apps(const std::function<void(const std::wstring& appID, 
    const wstring_set& clsIDs)>& fn)
{
    for (const auto& appid : m_appids) {
        wstring_set clsIDs;

        auto clsIt = m_appClsids.find(appid);
        if (clsIt != m_appClsids.end()) {
            clsIDs = clsIt->second;
        }

        fn(appid, clsIDs);
    }
}
