#pragma once
#include "coapps.h"
#include "cocats.h"

class CoObjects
{
public:
    CoObjects() = default;
    bool Construct();

    void classes(const std::function<void(
        const std::wstring& clsID,
        const std::wstring& appID,
        const wstring_set& catIDs)>& fn);

    void apps(const std::function<void(
        const std::wstring& appID,
        const wstring_set& clsIDs)>& fn);

private:
    wstring_set m_clsids;
    std::unordered_map<std::wstring, std::wstring> m_clsidApps;
    std::unordered_map<std::wstring, wstring_set> m_clsidCats;

    CoApps m_apps;
    CoCategories m_cats;
};
