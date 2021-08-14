#pragma once

class CoApps
{
public:
    CoApps() = default;
    bool Construct();

    void addClass(LPCWSTR appID, LPCWSTR clsID);
    void apps(const std::function<void(const std::wstring& appID,
                                       const wstring_set& clsIDs)>& fn);

private:
    wstring_set m_appids;
    std::unordered_map<std::wstring, wstring_set> m_appClsids;
};
