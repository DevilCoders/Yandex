#include "accumulator.h"
#include <library/cpp/xml/document/xml-document.h>

namespace NCS {
    namespace NLogging {
        void TDefaultLogsAccumulator::SerializeXmlReport(NXml::TNode result) const {
            for (auto&& i : Records) {
                i.SerializeToXml(result.AddChild("record"));
            }
        }

        TString TDefaultLogsAccumulator::GetStringReport() const {
            TStringStream ss;
            for (auto&& i : Records) {
                ss << i.SerializeToString() << Endl;
            }
            return ss.Str();
        }

        NJson::TJsonValue TDefaultLogsAccumulator::GetJsonReport() const {
            NJson::TJsonValue result = NJson::JSON_ARRAY;
            for (auto&& i : Records) {
                result.AppendValue(i.SerializeToJson());
            }
            return result;
        }

        void IEventLogsAccumulator::AddRecord(const TBaseLogRecord& r) {
            if (++RecordsCount <= RecordsLimit) {
                DoAddRecord(r);
            } else {
                TCSSignals::Signal("event_log_limit_reached");
            }
        }
    }
}
