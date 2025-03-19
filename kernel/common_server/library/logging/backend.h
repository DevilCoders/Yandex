#pragma once

#include <library/cpp/logger/backend.h>
#include <library/cpp/logger/backend_creator.h>

namespace NFrontend {

    class TEventLogBackend final: public TLogBackend {
    public:
        using TAdditionalFields = THashMap<TString, TString>;

        TEventLogBackend(const TAdditionalFields& additionalFields);
        virtual void WriteData(const ::TLogRecord& rec) override;
        virtual void ReopenLog() override;

    private:
        TAdditionalFields AdditionalFields;
    };

    class TEventLogBackendCreator final: public TLogBackendCreatorBase {
    public:
        TEventLogBackendCreator();
        virtual bool Init(const IInitContext& ctx) override;
        static TFactory::TRegistrator<TEventLogBackendCreator> Registrar;

    protected:
        virtual THolder<TLogBackend> DoCreateLogBackend() const override;
        virtual void DoToJson(NJson::TJsonValue& value) const override;

    private:
        TEventLogBackend::TAdditionalFields AdditionalFields;
    };
}
