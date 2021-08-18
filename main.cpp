#include "common.h"
#include "coobject.h"
#include "coobjects.h"
#include "ptable.h"
#include "timer.h"
#include "util.h"

using namespace std;
using namespace utility;

template <typename K, typename V>
void printStats(persistent_hash_table<K, V>& table)
{
    cout << endl << "    Index filename: " << table.indexname() << flush << endl;
    cout << "    Index file size: " << comma(table.indexsize()) << " bytes" << flush << endl;
    cout << "    Hash table size: " << comma(table.tablesize()) << " buckets" << flush << endl;
    cout << "    Hash table fill count: " << comma(table.fillcount()) << " buckets" << flush << endl;
    cout << "    Hash table load factor: " << boost::format("%02.2f%%") % table.loadfactor() << flush << endl;
    cout << "    Longest run: " << comma(table.maxrun()) << " buckets" << flush << endl << endl;
}

using namespace std;

static void testStore()
{
    persistent_hash_table<wstring, coobject> table;
    table.open("d:\\tmp\\coobjects.idx");

    CoObjects objects;
    objects.Construct();

    objects.classes([&table](const wstring& clsID,
                             const wstring& appID,
                             const wstring_set& catIDs)
    {
        coclass clazz(clsID, appID, catIDs);
        
        CString key;
        key.Format(L"CLSID:%s", clsID.c_str());

        auto result = table.insert(static_cast<LPCWSTR>(key), clazz);
        ASSERT(result);

        // VERIFY STEP!
        coobject o;
        result = table.lookup(static_cast<LPCWSTR>(key), o);
        ASSERT(result);

        coclass clazz2(std::move(o));

        ASSERT(clazz == clazz2);
    });

    objects.apps([&table](const wstring& appID,
                          const wstring_set& clsIDs)
    {
        coapp app(appID, clsIDs);

        CString key;
        key.Format(L"APPID:%s", appID.c_str());

        auto result = table.insert(static_cast<LPCWSTR>(key), app);
        ASSERT(result);

        // VERIFY STEP!
        coobject o;
        result = table.lookup(static_cast<LPCWSTR>(key), o);
        ASSERT(result);

        coapp app2(std::move(o));

        ASSERT(app == app2);
    });

    objects.cats([&table](const wstring& catID,
                          const wstring_set& clsIDs)
    {
        cocat cat(catID, clsIDs);

        CString key;
        key.Format(L"CATID:%s", catID.c_str());

        auto result = table.insert(static_cast<LPCWSTR>(key), cat);
        ASSERT(result);

        // VERIFY STEP!
        coobject o;
        result = table.lookup(static_cast<LPCWSTR>(key), o);
        ASSERT(result);

        cocat cat2(std::move(o));

        ASSERT(cat == cat2);
    });

    printStats(table);

    table.close();
}

int main()
{
    Timer timer;

    try {
        testStore();
    } catch (const exception& e) {
        cerr << e.what() << endl;
        return 1;
    }

    cout << "    elapsed time " << timer << flush << endl;

    return 0;
}
