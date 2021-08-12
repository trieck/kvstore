#include "common.h"

static void testStore()
{
    kvstore store;
    store.open(L"d:\\tmp\\tom.idx");

    coobject value(CoType::IID, GUID_NULL);
    store.insert("foo", value);

    coobject result;
    auto b = store.lookup("foo", result);
    assert(b);

    auto* ob1 = value.buffer();
    auto* ob2 = result.buffer();

    assert(ob1->type() == ob2->type());
    assert(ob1->guid()->str() == ob2->guid()->str());
}

int main()
{
    try {
        testStore();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
