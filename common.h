#pragma once

#if !defined(_UNICODE) || !defined(UNICODE)
#error Unicode must be defined
#endif

#include "kvstorelib.h"
#include <atlbase.h>
#include <atlstr.h>

using wstring_set = std::unordered_set<std::wstring>;

// ensure structured exceptions are enabled
#pragma warning(push)
#pragma warning(error:4535)
static void __check_eha()
{
    _set_se_translator(nullptr);
}
#pragma warning(pop)
