#include "common.h"
#include "coifaces.h"
#include "regkey.h"

bool CoInterfaces::Construct()
{
    RegKey key;

    auto result = key.Open(HKEY_CLASSES_ROOT, L"Interface", KEY_READ);
    if (result != ERROR_SUCCESS) {
        return false;
    }

    DWORD index = 0;
    CString keyName;

    for (;;) {
        result = key.EnumKey(index++, keyName);
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }

        if (result != ERROR_SUCCESS) {
            continue;
        }
    }

    return true;
}
