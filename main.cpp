#include "common.h"
#include "coobjects.h"
#include "coobject.h"
#include "kvstore.h"

struct CoInit
{
    CoInit()
    {
        CoInitialize(nullptr);
    }

    ~CoInit()
    {
        CoUninitialize();
    }
};

static void testStore()
{
    kvstore store;
    store.open(L"d:\\tmp\\coobjects.idx");

    CoObjects objects;
    objects.Construct();

    objects.classes([&store](const std::wstring& clsID,
                             const std::wstring& appID,
                             const wstring_set& catIDs)
    {
        coclass clazz(clsID, appID, catIDs);

        CString key;
        key.Format(L"CLSID:%s", clsID.c_str());

        store.insert(key, clazz);
    });

    objects.apps([&store](const std::wstring& appID,
                          const wstring_set& clsIDs)
    {
        coapp app(appID, clsIDs);

        CString key;
        key.Format(L"APPID:%s", appID.c_str());

        store.insert(key, app);
    });

    store.close();
}

int main()
{
    CoInit init;

    try {
        testStore();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
