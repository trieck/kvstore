#pragma once

class CoCategories
{
public:
    CoCategories() = default;
    bool Construct();

    void addClass(LPCWSTR catID, LPCWSTR clsID);

private:
    using wstring_set = std::unordered_set<std::wstring>;

    wstring_set m_catids;
    std::unordered_map<std::wstring, wstring_set> m_catClsids;
};

