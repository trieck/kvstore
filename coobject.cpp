#include "common.h"
#include "coobject.h"
#include "utf8str.h"

using namespace flatbuffers;

coclass::coclass(const std::wstring& clsID, const std::wstring& appID,
                 const std::unordered_set<std::wstring>& catIDs)
{
    auto sCLSID = utf8str(clsID);
    auto guidOffset = CreateString(static_cast<LPCSTR>(sCLSID));

    Offset<String> appidOffset;
    if (!appID.empty()) {
        auto sAppID = utf8str(appID);
        appidOffset = CreateString(static_cast<LPCSTR>(sAppID));
    }

    Offset<Vector<Offset<String>>> catidsOffset;
    if (!catIDs.empty()) {
        std::vector<std::string> vCatIDs;
        for (const auto& catID : catIDs) {
            vCatIDs.emplace_back(utf8str(catID));
        }

        catidsOffset = CreateVectorOfStrings(vCatIDs);
    }

    CoClassBuilder cbuilder(*this);
    if (!appidOffset.IsNull()) {
        cbuilder.add_app_id(appidOffset);
    }

    if (!catidsOffset.IsNull()) {
        cbuilder.add_cat_ids(catidsOffset);
    }

    auto coclass = cbuilder.Finish().Union();

    CoObjectBuilder builder(*this);
    builder.add_type_type(CoType::CoClass);
    builder.add_type(coclass);
    builder.add_guid(guidOffset);

    auto root = builder.Finish();
    FinishCoObjectBuffer(*this, root);
}

coapp::coapp(const std::wstring& appID, const wstring_set& clsIDs)
{
    auto sAppID = utf8str(appID);
    auto guidOffset = CreateString(static_cast<LPCSTR>(sAppID));

    Offset<Vector<Offset<String>>> clsidsOffset;
    if (!clsIDs.empty()) {
        std::vector<std::string> vClsIDs;
        for (const auto& clsID : clsIDs) {
            vClsIDs.emplace_back(utf8str(clsID));
        }

        clsidsOffset = CreateVectorOfStrings(vClsIDs);
    }

    CoAppBuilder abuilder(*this);
    if (!clsidsOffset.IsNull()) {
        abuilder.add_cls_ids(clsidsOffset);
    }
    auto coapp = abuilder.Finish().Union();

    CoObjectBuilder builder(*this);
    builder.add_type_type(CoType::CoApp);
    builder.add_type(coapp);
    builder.add_guid(guidOffset);

    auto root = builder.Finish();
    FinishCoObjectBuffer(*this, root);
}
