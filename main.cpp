#include "common.h"

#include "coinit.h"
#include "coobject.h"
#include "coobjects.h"
#include "kvstore.h"
#include "timer.h"
#include "util.h"

using namespace std;
using namespace utility;

static void printStats(kvstore& store)
{
    wcout << endl << L"    Index filename: " << store.indexname() << flush << endl;

    cout << "    Index file size: " << comma(store.indexsize()) << " bytes" << flush << endl;
    cout << "    Hash table size: " << comma(store.tablesize()) << " buckets" << flush << endl;
    cout << "    Hash table fill count: " << comma(store.fillcount()) << " buckets" << flush << endl;
    cout << "    Hash table load factor: " << boost::format("%02.2f%%") % store.loadfactor() << flush << endl;
    cout << "    Longest run: " << comma(store.maxrun()) << " buckets" << flush << endl << endl;
}

static void testStore()
{
    kvstore store;
    store.open(L"d:\\tmp\\coobjects.idx");

    CoObjects objects;
    objects.Construct();

    objects.classes([&store](const wstring& clsID,
                             const wstring& appID,
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

        coclass clazz2(o);

        ASSERT(clazz == clazz2);
    });

    objects.apps([&store](const wstring& appID,
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

    objects.cats([&store](const wstring& catID,
                          const wstring_set& clsIDs)
    {
        cocat cat(catID, clsIDs);

        CString key;
        key.Format(L"CATID:%s", catID.c_str());

        auto result = store.insert(key, cat);
        ASSERT(result);

        // VERIFY STEP!
        coobject o;
        result = store.lookup(key, o);
        ASSERT(result);
        ASSERT(cat == o);
    });

    auto key = L"CLSID:{60F75E71-0039-11D1-B1C9-000000000000}";

    coclass clazz(key);
    auto result = store.insert(key, clazz);
    ASSERT(!result); // already there

    coobject object;
    result = store.lookup(key, object);
    ASSERT(result);

    key = L"APPID:{79426537-5AA0-4D44-A2F4-999B148AE0AD}";
    coapp app(key);

    result = store.insert(key, app);
    ASSERT(!result); // already there

    result = store.lookup(key, object);
    ASSERT(result);

    printStats(store);

    store.close();
}

int main()
{
    Timer timer;
    CoInit init;

    try {
        testStore();
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return 1;
    }

    cout << "    elapsed time " << timer << flush << endl;

    return 0;
}
