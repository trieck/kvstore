#include "common.h"
#include "CoObjects.h"
#include "regkey.h"

bool CoObjects::Construct()
{
    m_apps.Construct();
    m_cats.Construct();

    RegKey key, subKey;
    auto result = key.Open(HKEY_CLASSES_ROOT, L"CLSID", KEY_READ);
    if (result != ERROR_SUCCESS) {
        return false;
    }

    DWORD index = 0;
    CString clsID;

    for (;;) {
        result = key.EnumKey(index++, clsID);
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (result != ERROR_SUCCESS) {
            continue;
        }

        result = subKey.Open(key, clsID, KEY_READ);
        if (result != ERROR_SUCCESS) {
            continue;
        }

        clsID.MakeUpper();

        m_clsids.insert(static_cast<LPCWSTR>(clsID));

        CString appID;
        result = subKey.QueryStringValue(L"AppID", appID);
        if (result == ERROR_SUCCESS) {
            appID.MakeUpper();
            m_clsidApps[static_cast<LPCWSTR>(clsID)] = appID;
            m_apps.addClass(appID, clsID);
        }

        result = subKey.Open(subKey, L"Implemented Categories", KEY_READ);
        if (result != ERROR_SUCCESS) {
            continue;
        }

        CString catID;
        auto catIndex = 0;
        wstring_set catIDs;

        for (;;) {
            result = subKey.EnumKey(catIndex++, catID);
            if (result == ERROR_NO_MORE_ITEMS) {
                break;
            }

            if (result != ERROR_SUCCESS) {
                continue;
            }

            catID.MakeUpper();
            catIDs.insert(static_cast<LPCWSTR>(catID));
            m_cats.addClass(catID, clsID);
        }

        m_clsidCats[static_cast<LPCWSTR>(clsID)] = catIDs;
    }

    return true;
}

void CoObjects::classes(const std::function<void(const std::wstring&,
                                                 const std::wstring&,
                                                 const wstring_set&)>& fn)
{
    for (const auto& clsid : m_clsids) {
        std::wstring appID;
        wstring_set catIDs;

        auto appIt = m_clsidApps.find(clsid);
        if (appIt != m_clsidApps.end()) {
            appID = appIt->second;
        }

        auto catIt = m_clsidCats.find(clsid);
        if (catIt != m_clsidCats.end()) {
            catIDs = catIt->second;
        }

        fn(clsid, appID, catIDs);
    }
}

void CoObjects::apps(const std::function<void(const std::wstring& appID,
                                              const wstring_set& clsIDs)>& fn)
{
    m_apps.apps(fn);
}
