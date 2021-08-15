#pragma once

class CoCategories
{
public:
    CoCategories() = default;
    bool Construct();

    void addClass(LPCWSTR catID, LPCWSTR clsID);
    void cats(const std::function<void(const std::wstring& catID,
                                       const std::unordered_set<std::wstring>& clsIDs)>& fn);

private:
    using wstring_set = std::unordered_set<std::wstring>;

    wstring_set m_catids;
    std::unordered_map<std::wstring, wstring_set> m_catClsids;
};
