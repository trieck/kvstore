#include "kvstorelib.h"
#include "utf8str.h"

utf8str::utf8str(const std::wstring& input)
    : utf8str(input.c_str())
{
}

utf8str::utf8str(LPCWSTR input)
{
    int nLengthW = static_cast<int>(wcslen(input));
    int nLengthA = nLengthW * 4;

    m_str.resize(nLengthA);

    auto result = WideCharToMultiByte(CP_UTF8, 0, input, nLengthW, &m_str[0], 
        nLengthA, nullptr, nullptr);

    m_str.resize(result);
}

utf8str::operator LPCSTR() const
{
    return m_str.c_str();
}

utf8str::operator const std::string&() const
{
    return m_str;
}

const std::string& utf8str::str() const
{
    return m_str;
}
