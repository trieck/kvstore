#include "common.h"
#include "cocats.h"
#include "regkey.h"

bool CoCategories::Construct()
{
    RegKey key;

    auto result = key.Open(HKEY_CLASSES_ROOT, L"Component Categories", KEY_READ);
    if (result != ERROR_SUCCESS) {
        return false;
    }

    DWORD index = 0;
    CString catID;

    for (;;) {
        result = key.EnumKey(index++, catID);
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (result != ERROR_SUCCESS) {
            continue;
        }
        
        m_catids.insert(static_cast<LPCWSTR>(catID));
    }

    return true;
}

void CoCategories::addClass(LPCWSTR catID, LPCWSTR clsID)
{
    m_catids.insert(catID);

    auto& it = m_catClsids[catID];
    it.insert(clsID);
}

void CoCategories::cats(const std::function<void(const std::wstring& catID,
                                                 const std::unordered_set<std::wstring>& clsIDs)>& fn)
{
    for (const auto& catid : m_catids) {
        wstring_set clsIDs;

        auto clsIt = m_catClsids.find(catid);
        if (clsIt != m_catClsids.end()) {
            clsIDs = clsIt->second;
        }

        fn(catid, clsIDs);
    }
}
