#include "common.h"

int main()
{
    coobject object(CoType::IID, GUID_NULL);

    kvstore store;

    try {
        store.open(L"d:\\tmp\\tom.idx");
        store.insert("foo");

    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
