#include "manager.h"

#include <library/cpp/tvmauth/client/facade.h>

NFrontend::TScheme TNotifierContainer::GetScheme(const IBaseServer& server) {
    NFrontend::TScheme result;
    result.Add<TFSNumeric>("revision", "Версия конфигурации").SetReadOnly(true);
    result.Add<TFSString>("name", "Идентификатор процесса");
    result.Add<TFSBoolean>("notifier_is_active", "Нотификатор активен").SetReadOnly(true);
    result.Add<TFSWideVariants>("type", "Тип нотификации").InitVariants<IFrontendNotifierConfig>(server).SetCustomStructureId("meta");
    return result;
}

bool TNotifierContainer::DeserializeWithDecoder(const TDecoder& decoder, const TConstArrayRef<TStringBuf>& values) {
    READ_DECODER_VALUE(decoder, values, Revision);
    READ_DECODER_VALUE(decoder, values, Name);
    if (!Name) {
        return false;
    }
    TString typeName;
    READ_DECODER_VALUE_TEMP(decoder, values, typeName, Type);
    NotifierConfig = IFrontendNotifierConfig::TFactory::Construct(typeName);
    NJson::TJsonValue jsonMeta;
    READ_DECODER_VALUE_JSON(decoder, values, jsonMeta, Meta);
    if (!NotifierConfig || !NotifierConfig->DeserializeFromJson(jsonMeta)) {
        return false;
    }
    NotifierConfig->SetTypeName(typeName);
    NotifierConfig->SetName(Name);
    Notifier = NotifierConfig->Construct();
    if (!Notifier) {
        return false;
    }
    return true;
}

NJson::TJsonValue TNotifierContainer::GetReport() const {
    NJson::TJsonValue result;
    result.InsertValue("name", Name);
    result.InsertValue("type", NotifierConfig->GetTypeName());
    result.InsertValue("meta", NotifierConfig->SerializeToJson());
    result.InsertValue("notifier_is_active", !!Notifier && Notifier->IsActive());
    if (HasRevision()) {
        result.InsertValue("revision", Revision);
    }
    return result;
}

NStorage::TTableRecord TNotifierContainer::SerializeToTableRecord() const {
    CHECK_WITH_LOG(!!NotifierConfig);
    NStorage::TTableRecord result;
    result.Set("name", Name);
    result.Set("type", NotifierConfig->GetTypeName());
    result.Set("meta", NotifierConfig->SerializeToJson());
    if (HasRevision()) {
        result.Set("revision", GetRevision());
    }
    return result;
}

NJson::TJsonValue TNotifierContainer::SerializeToJson() const {
    return SerializeToTableRecord().BuildWT().SerializeToJson();
}

void TNotifierContainer::Start(const IBaseServer& server) {
    if (Notifier) {
        if (const auto& clientId = Notifier->GetTvmClientName()) {
            Notifier->SetTvmClient(server.GetTvmManager()->GetTvmClient(*clientId));
        }
        Notifier->Start(server);
    }
}
