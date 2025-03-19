#include "configured.h"

TConfiguredRolesManagerConfig::TFactory::TRegistrator<TConfiguredRolesManagerConfig> TConfiguredRolesManagerConfig::Registrator(TConfiguredRolesManagerConfig::GetTypeName());

TItemPermissionContainer TConfiguredRolesManagerConfig::GetItem(const TString& itemId) const {
    TReadGuard g(Mutex);
    auto it = Items.find(itemId);
    if (it != Items.end()) {
        return it->second;
    }
    return TItemPermissionContainer();
}

void TConfiguredRolesManagerConfig::DoToString(IOutputStream& os) const {
    os << "<Items>" << Endl;
    for (auto [itemId, item] : Items) {
        os << "<" << itemId << ">" << Endl;
        item->ToString(os);
        os << "</" << itemId << ">" << Endl;
    }
    os << "</Items>" << Endl;
    os << "<Roles>" << Endl;
    for (auto [roleId, info] : InfoByRole) {
        os << "<" << roleId << ">" << Endl;
        os << "Items: " << JoinSeq(", ", info.first) << Endl;
        os << "Roles: " << JoinSeq(", ", info.second) << Endl;
        os << "</" << roleId << ">" << Endl;
    }
    os << "</Roles>" << Endl;
}

void TConfiguredRolesManagerConfig::DoInit(const TYandexConfig::Section* section) {
    auto children = section->GetAllChildren();
    {
        auto it = children.find("Items");
        if (it != children.end()) {
            for (auto [itemId, section] : it->second->GetAllChildren()) {
                auto type = section->GetDirectives().Value<TString>("Type", "");
                TItemPermissionContainer item(IItemPermissions::TFactory::Construct(type));
                AssertCorrectConfig(!!item, "Incorrect item type %s", type.data());
                item.SetItemId(itemId);
                item->Init(section);
                Items.emplace(itemId, item);
            }
        }
    }
    {
        auto it = children.find("Roles");
        if (it != children.end()) {
            for (auto [roleId, section] : it->second->GetAllChildren()) {
                TVector<TString> items;
                StringSplitter(section->GetDirectives().Value<TString>("Items", "")).SplitBySet(", ").SkipEmpty().Collect(&items);
                TVector<TString> roles;
                StringSplitter(section->GetDirectives().Value<TString>("Roles", "")).SplitBySet(", ").SkipEmpty().Collect(&roles);
                InfoByRole.emplace(roleId, std::make_pair(std::move(items), std::move(roles)));
            }
        }
    }
}

THolder<IRolesManager> TConfiguredRolesManagerConfig::BuildManager(const IBaseServer& /*server*/) const {
    return MakeHolder<TConfiguredRolesManager<>>(*this);
}

