#include "backend.h"
#include "events.h"
#include <util/string/split.h>
#include <util/string/join.h>

namespace NFrontend {


    TEventLogBackend::TEventLogBackend(const TAdditionalFields& additionalFields)
        : AdditionalFields(additionalFields)
    {}

    void TEventLogBackend::WriteData(const ::TLogRecord& rec) {
        auto r = TFLEventLog::Log(TString(rec.Data, rec.Len), rec.Priority);
        for(const auto& [k, v]: AdditionalFields) {
            r(k, v);
        }
    }

    void TEventLogBackend::ReopenLog() {
    }

    ILogBackendCreator::TFactory::TRegistrator<TEventLogBackendCreator> TEventLogBackendCreator::Registrar("events");

    TEventLogBackendCreator::TEventLogBackendCreator()
        : TLogBackendCreatorBase("events")
    {}


    bool TEventLogBackendCreator::Init(const IInitContext& ctx) {
        if (!StringSplitter(ctx.GetOrElse("AdditionalFields", TString())).SplitBySet(", ").SkipEmpty().Consume([this](const TStringBuf pair) {
                TStringBuf k, v;
                pair.Split("=", k, v);
                if (k && v) {
                    AdditionalFields[k] = v;
                } else {
                    ERROR_LOG << "incorrect additional field " << pair << Endl;
                    return false;
                }
                return true;
            }))
        {
            return false;
        };
        return true;
    }

    THolder<TLogBackend> TEventLogBackendCreator::DoCreateLogBackend() const {
        return MakeHolder<TEventLogBackend>(AdditionalFields);
    }

    void TEventLogBackendCreator::DoToJson(NJson::TJsonValue& value) const {
        TVector<TString> fields;
        for (const auto& [k, v] : AdditionalFields) {
            fields.emplace_back(k + "=" + v);
        }
        value["AdditionalFields"] = JoinSeq(",", fields);
    }

}
