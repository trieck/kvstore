#include "common.h"
#include "coobject.h"
#include "utf8str.h"

using namespace flatbuffers;

CoType coobject::type() const
{
    return buffer().type_type();
}

std::string coobject::guid() const
{
    auto* pguid = buffer().guid();
    if (pguid == nullptr) {
        return "";
    }

    return pguid->str();
}

void coobject::copy(const uint8_t* data, uint32_t size)
{
    m_builder.Reset();
    m_builder.PushFlatBuffer(data, size);
}

uint8_t* coobject::data() const
{
    return m_builder.GetBufferPointer();
}

uint32_t coobject::size() const
{
    return m_builder.GetSize();
}

bool coobject::operator==(const IValue& rhs)
{
    if (size() != rhs.size()) {
        return false;
    }

    return memcmp(data(), rhs.data(), size()) == 0;
}

const CoObject& coobject::buffer() const
{
    return *GetCoObject(m_builder.GetBufferPointer());
}

coclass::coclass(const std::wstring& clsID, const std::wstring& appID,
                 const std::unordered_set<std::wstring>& catIDs)
{
    auto sCLSID = utf8str(clsID);
    auto guidOffset = m_builder.CreateString(static_cast<LPCSTR>(sCLSID));

    Offset<String> appidOffset;
    if (!appID.empty()) {
        auto sAppID = utf8str(appID);
        appidOffset = m_builder.CreateString(static_cast<LPCSTR>(sAppID));
    }

    Offset<Vector<Offset<String>>> catidsOffset;
    if (!catIDs.empty()) {
        std::vector<std::string> vCatIDs;
        for (const auto& catID : catIDs) {
            vCatIDs.emplace_back(utf8str(catID));
        }

        catidsOffset = m_builder.CreateVectorOfStrings(vCatIDs);
    }

    CoClassBuilder cbuilder(m_builder);
    if (!appidOffset.IsNull()) {
        cbuilder.add_app_id(appidOffset);
    }

    if (!catidsOffset.IsNull()) {
        cbuilder.add_cat_ids(catidsOffset);
    }

    auto coclass = cbuilder.Finish().Union();

    CoObjectBuilder builder(m_builder);
    builder.add_type_type(CoType::CoClass);
    builder.add_type(coclass);
    builder.add_guid(guidOffset);

    auto root = builder.Finish();
    FinishCoObjectBuffer(m_builder, root);
}

LPCSTR coclass::appID() const
{
    auto* pClass = as<CoClass>();
    ASSERT(pClass != nullptr);

    auto* pAppID = pClass->app_id();
    if (pAppID == nullptr) {
        return nullptr;
    }

    return pAppID->c_str();
}

coapp::coapp(const std::wstring& appID, const wstring_set& clsIDs)
{
    auto sAppID = utf8str(appID);
    auto guidOffset = m_builder.CreateString(static_cast<LPCSTR>(sAppID));

    Offset<Vector<Offset<String>>> clsidsOffset;
    if (!clsIDs.empty()) {
        std::vector<std::string> vClsIDs;
        for (const auto& clsID : clsIDs) {
            vClsIDs.emplace_back(utf8str(clsID));
        }

        clsidsOffset = m_builder.CreateVectorOfStrings(vClsIDs);
    }

    CoAppBuilder abuilder(m_builder);
    if (!clsidsOffset.IsNull()) {
        abuilder.add_cls_ids(clsidsOffset);
    }
    auto coapp = abuilder.Finish().Union();

    CoObjectBuilder builder(m_builder);
    builder.add_type_type(CoType::CoApp);
    builder.add_type(coapp);
    builder.add_guid(guidOffset);

    auto root = builder.Finish();
    FinishCoObjectBuffer(m_builder, root);
}

cocat::cocat(const std::wstring& catID, const wstring_set& clsIDs)
{
    auto sCatID = utf8str(catID);
    auto guidOffset = m_builder.CreateString(static_cast<LPCSTR>(sCatID));

    Offset<Vector<Offset<String>>> clsidsOffset;
    if (!clsIDs.empty()) {
        std::vector<std::string> vClsIDs;
        for (const auto& clsID : clsIDs) {
            vClsIDs.emplace_back(utf8str(clsID));
        }

        clsidsOffset = m_builder.CreateVectorOfStrings(vClsIDs);
    }

    CoCategoryBuilder cbuilder(m_builder);
    if (!clsidsOffset.IsNull()) {
        cbuilder.add_cls_ids(clsidsOffset);
    }
    auto cocat = cbuilder.Finish().Union();

    CoObjectBuilder builder(m_builder);
    builder.add_type_type(CoType::CoCategory);
    builder.add_type(cocat);
    builder.add_guid(guidOffset);

    auto root = builder.Finish();
    FinishCoObjectBuffer(m_builder, root);
}
