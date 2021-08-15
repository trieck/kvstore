#include "common.h"
#include "coobjects.h"
#include "coobject.h"
#include "kvstore.h"
#include "utf8str.h"

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
    store.open(L"d:\\tmp\\coobjects.idx", 2);

    CoObjects objects;
    objects.Construct();

    objects.classes([&store](const std::wstring& clsID,
                             const std::wstring& appID,
                             const wstring_set& catIDs)
    {
        coclass clazz(clsID, appID, catIDs);

        CString key;
        key.Format(L"CLSID:%s", clsID.c_str());

        auto result = store.insert(key, clazz);
        ASSERT(result);

        // VERIFY STEP!
        coobject o;
        result = store.lookup(key, o);
        ASSERT(result);
        ASSERT(clazz == o);
    });

    objects.apps([&store](const std::wstring& appID,
                          const wstring_set& clsIDs)
    {
        coapp app(appID, clsIDs);

        CString key;
        key.Format(L"APPID:%s", appID.c_str());

        auto result = store.insert(key, app);
        ASSERT(result);

        // VERIFY STEP!
        coobject o;
        result = store.lookup(key, o);
        ASSERT(result);
        ASSERT(app == o);
    });

    //// TODO: Categories
    auto key = L"CLSID:{60F75E71-0039-11D1-B1C9-000000000000}";

    coclass clazz(key);
    auto result = store.insert(key, clazz);
    ASSERT(!result);    // already there

    coobject object;
    result = store.lookup(key, object);
    ASSERT(result);

    key = L"APPID:{79426537-5AA0-4D44-A2F4-999B148AE0AD}";
    coapp app(key);

    result = store.insert(key, app);
    ASSERT(!result);    // already there

    result = store.lookup(key, object);
    ASSERT(result);
    
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
