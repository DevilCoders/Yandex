#include "manager.h"
#include "abstract.h"

TTagDescriptionsManager::TTagDescriptionsManager(THolder<IHistoryContext>&& context, const TTagDescriptionsManagerConfig& config, const IBaseServer& server)
    : TBase(std::move(context), config.GetHistoryConfig())
    , Server(server) {
    Singleton<TTagDescriptionsOperator>()->Register(this);

    auto session = BuildNativeSession(false);
    for (auto&& i : ITagDescription::TFactory::GetRegisteredKeys()) {
        THolder<ITagDescription> td(ITagDescription::TFactory::Construct(i));
        CHECK_WITH_LOG(!!td);
        for (auto&& tdImpl : td->GetDefaultObjects()) {
            NStorage::TTableRecord condition;
            condition.Set("name", tdImpl.GetName());
            condition.Set("class_name", tdImpl->GetClassName());
            TMaybe<TDBTagDescription> tdLocal;
            CHECK_WITH_LOG(AddRecord(tdImpl.SerializeToTableRecord(), "default_constructor", session, &tdLocal, &condition));
        }
    }
    CHECK_WITH_LOG(session.Commit());

    Y_UNUSED(Server);
}

TTagDescriptionsManager::~TTagDescriptionsManager() {
    Singleton<TTagDescriptionsOperator>()->Unregister();
}

bool TTagDescriptionsManager::UpsertTagDescriptions(const TVector<TDBTagDescription>& tDescriptions, const TString& userId, NCS::TEntitySession& session) const {
    for (auto&& i : tDescriptions) {
        if (!i) {
            return false;
        }
        NStorage::TTableRecord update = i.SerializeToTableRecord();
        NStorage::TTableRecord condition;
        condition.SetNotEmpty("revision", i.GetRevision());
        condition.Set("name", i.GetName());
        condition.Set("class_name", i->GetClassName());
        if (!UpsertObject(i, userId, session)) {
            return false;
        }
    }
    return true;
}

bool TTagDescriptionsManager::RemoveTagDescriptions(const TSet<TString>& tagNames, const TString& userId, const bool removeDeprecated, NCS::TEntitySession& session) const {
    if (tagNames.empty()) {
        return true;
    }
    TVector<TDBTagDescription> descriptions;
    if (!RestoreObjectsBySRCondition(descriptions, TSRCondition().Init<TSRBinary>("name", tagNames), session)) {
        return false;
    }
    for (auto&& i : descriptions) {
        NStorage::TTableRecord condition;
        condition.Set("name", i.GetName());
        if (i.GetDeprecated()) {
            if (removeDeprecated && !RemoveRecord(condition, userId, session, nullptr)) {
                return false;
            }
        } else {
            i.SetDeprecated(true);
            if (!UpdateRecord(i.SerializeToTableRecord(), condition, userId, session, nullptr)) {
                return false;
            }
        }
    }
    return true;
}

