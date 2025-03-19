#include "record.h"
#include <library/cpp/xml/document/node-attr.h>
#include <util/system/tls.h>
#include <util/string/escape.h>
#include <util/stream/str.h>
#include <util/system/backtrace.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/util/algorithm/iterator.h>
#include <library/cpp/json/writer/json.h>
#include <util/string/subst.h>
#include <util/random/random.h>
#include <util/system/thread.h>

namespace {
    Y_STATIC_THREAD(TDeque<const NCS::NLogging::TLogThreadContext*>) Contexts;
}

template <>
void Out<NCS::NLogging::TBaseLogRecord>(IOutputStream& out, const NCS::NLogging::TBaseLogRecord& value) {
    out << value.SerializeToString();
}

namespace NCS {
    namespace NLogging {
        ui32 TEventLogSettings::LineLengthLimit = 1024;

        TLogThreadContext::~TLogThreadContext() {
            CHECK_WITH_LOG(::Contexts.Get().size());
            CHECK_WITH_LOG(::Contexts.Get().back()->GetContextId() == GetContextId())
                << GetContextId() << ":" << SerializeToString() << "/"
                << ::Contexts.Get().back()->GetContextId() << ":" << ::Contexts.Get().back()->SerializeToString()
                << Endl;
            if (::Contexts.Get().size()) {
                ::Contexts.Get().pop_back();
            }
        }

        TLogThreadContext::TGuard TLogRecordConstructor::StartContext() {
            auto result = MakeHolder<TLogThreadContext>();
            if (::Contexts.Get().empty()) {
                (*result)("_ctx_id", result->GetContextId());
            }
            ::Contexts.Get().emplace_back(result.Get());
            return TLogThreadContext::TGuard(result.Release());
        }

        TLogRecord TLogRecordConstructor::BaseRecord() {
            TLogRecord result;
            result.InitBaseRecord();
            return result;
        }

        TLogRecord& TLogRecordConstructor::FillContext(TLogRecord& record) {
            for (auto&& i : ::Contexts.Get()) {
                record(*i);
            }
            return record;
        }

        TSignalTagsSet TLogRecordConstructor::CollectSignalTags() {
            TSignalTagsSet result;
            for (auto&& i : ::Contexts.Get()) {
                for (auto&& p : *i) {
                    if (p.IsSignalTag()) {
                        result(p.GetName(), p.GetValue());
                    }
                }
            }
            return result;
        }

        TLogRecord TLogRecordConstructor::Record() {
            TLogRecord result = BaseRecord();
            FillContext(result);
            return result;
        }

        TLogRecord TLogRecordConstructor::Record(const TString& eventInfo, const ELogPriority priority) {
            TLogRecord result = Record();
            if (!!eventInfo) {
                result("text", eventInfo);
            }
            result.SetPriority(priority);
            return result;
        }

        void TBaseLogRecord::PrepareFields() const {
            if (WrotePriority == -1 || (ELogPriority)WrotePriority != Priority) {
                Fields.emplace_back(TLogRecordField("level", ::ToString(Priority)));
            }
            if (SortedCount == Fields.size()) {
                return;
            }
            std::stable_sort(Fields.begin(), Fields.end());
            WrotePriority = (i32)Priority;
            SortedCount = Fields.size();
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Lowest(const TString& text) noexcept {
            return Add("text", text).SetPriority(TLOG_RESOURCES);
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Debug(const TString& text) noexcept {
            return Add("text", text).SetPriority(TLOG_DEBUG);
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Info(const TString& text) noexcept {
            return Add("text", text).SetPriority(TLOG_INFO);
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Notice(const TString& text) noexcept {
            return Add("text", text).SetPriority(TLOG_NOTICE);
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Warning(const TString& text) noexcept {
            return Add("text", text).SetPriority(TLOG_WARNING);
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Error(const TString& text) noexcept {
            return Add("text", text).SetPriority(TLOG_ERR);
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Critical(const TString& text) noexcept {
            return Add("text", text).SetPriority(TLOG_CRIT);
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Add(const TString& key, const TString& valueExt, const ui32 logLineLimit) noexcept {
            if (!valueExt) {
                return *this;
            }
            const TString sb = valueExt.substr(0, logLineLimit);
            Fields.emplace_back(TLogRecordField(key, sb));
            return *this;
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Add(const TString& key, const NJson::TMapBuilder& value, const ui32 logLineLimit /*= TEventLogSettings::LineLengthLimit*/) {
            Add(key, value.GetJson().GetStringRobust(), logLineLimit);
            return *this;
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Merge(const TBaseLogRecord& record) {
            IsAlert |= record.IsAlert;
            if (SignalType == ESignalType::NoSignal) {
                SignalType = record.SignalType;
            }
            Priority = ELogPriority(Min<int>(Priority, record.Priority));
            Fields.insert(Fields.end(), record.Fields.begin(), record.Fields.end());
            return *this;
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::AddEscaped(const TString& key, const TString& valueExt, const ui32 logLineLimit) {
            if (valueExt.find('\n') != TString::npos) {
                return Add(key, EscapeC(valueExt), logLineLimit);
            }
            return Add(key, valueExt, logLineLimit);
        }

        void TBaseLogRecord::InitBaseRecord() {
            const TInstant now = Now();
            Add("timestamp", ToStringLocalWithMs(now));
            Add("unixtime", now.Seconds());
            Add("evstamp", now.MicroSeconds());
            Add("host", HostName());
            Add("_tid", TThread::CurrentThreadId());
        }

        TString TBaseLogRecord::ToStringLocalWithMs(const TInstant& instant) const {
            return instant.FormatLocalTime("%Y-%m-%dT%H:%M:%S")
                + ToString(instant.MicroSecondsOfSecond() + 1000000).replace(0, 1, 1, '.');
        }

        void TBaseLogRecord::DoSignal() const {
            if (SignalType == ESignalType::NoSignal) {
                return;
            }
            TString signalId;
            for (auto&& i : Fields) {
                if (i.GetName() == "unistat_signal") {
                    signalId = i.GetValue();
                }
            }
            if (signalId) {
                TSignalTagsSet tags;
                for (auto&& i : Fields) {
                    if (i.IsSignalTag()) {
                        tags.Force(i.GetName(), i.GetValue());
                    }
                }
                switch (SignalType) {
                    case ESignalType::Counter:
                        TCSSignals::LSignal(signalId, Value)(tags);
                        break;
                    case ESignalType::CounterWithTimeout:
                        TCSSignals::LTSignal(signalId, Value)(tags);
                        break;
                    case ESignalType::Increment:
                        TCSSignals::Signal(signalId, Value)(tags);
                        break;
                    case ESignalType::NoSignal:
                        break;
                }
            }
        }

        TBaseLogRecord& TBaseLogRecord::SignalDetails(const TString& signalName, const double value, const ESignalType signalType) {
            Value = value;
            if (!!signalName) {
                Add("*unistat_signal", signalName);
            }
            Add("*_signal_value", value);
            SignalType = signalType;
            return *this;
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Signal(const TString& signalName /*= ""*/, const double value /*= 1*/) {
            return SignalDetails(signalName, value, ESignalType::Increment);
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::LTSignal(const TString& signalName /*= ""*/, const double value /*= 0*/) {
            return SignalDetails(signalName, value, ESignalType::CounterWithTimeout);
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::LSignal(const TString& signalName /*= ""*/, const double value /*= 0*/) {
            return SignalDetails(signalName, value, ESignalType::Counter);
        }

        NCS::NLogging::TBaseLogRecord& TBaseLogRecord::Alert(const TString& alertNotifierId /*= ""*/) {
            AlertNotifierId = alertNotifierId;
            IsAlert = true;
            SetPriority(ELogPriority::TLOG_ALERT);
            return Trace(true);
        }

        NJson::TJsonValue TBaseLogRecord::SerializeToJson() const {
            NJson::TJsonValue result = NJson::JSON_MAP;
            const auto pred = [&result](const TString& key, const TString& value) {
                result.InsertValue(key, value);
            };
            ScanFields(pred);
            return result;
        }

        TBaseLogRecord& TBaseLogRecord::Trace(const bool escape) {
            TString strTrace;
            {
                TStringOutput sb(strTrace);
                FormatBackTrace(&sb);
            }
            return Add("*trace", escape ? EscapeC(strTrace) : strTrace);
        }

        void TBaseLogRecord::SerializeToXml(NXml::TNode result) const {
            const auto pred = [&result](const TString& key, const TString& value) {
                result.AddChild(key, value);
            };
            ScanFields(pred);
        }

        bool TBaseLogRecord::IsAvailableField(const TLogRecordField& f) const {
            if (f.IsSignalName()) {
                return IsSignal();
            }
            return true;
        }

        TString TBaseLogRecord::SerializeToString(const ELogRecordFormat format /*= ELogRecordFormat::TSKV*/) const {
            TString predKey;
            if (format == ELogRecordFormat::TSKV) {
                TTSKVStreamRec s;
                const auto pred = [&s](const TString& key, const TString& value) {
                    s.Add(key, value);
                };
                ScanFields(pred);
                return s.ToString();
            } else if (format == ELogRecordFormat::Json) {
                NJsonWriter::TBuf s;
                s.BeginObject();
                const auto pred = [&s](const TString& key, const TString& value) {
                    s.WriteKey(key).WriteString(value);
                };
                ScanFields(pred);
                s.EndObject();
                return s.Str();
            } else if (format == ELogRecordFormat::HR) {
                TStringBuilder ss;
                for (auto&& i : Fields) {
                    if (!IsAvailableField(i)) {
                        continue;
                    }
                    ss << i.GetName() << ": " << i.GetValue() << Endl;
                }
                return ss;
            }
            return "";
        }

        TLogRecordField::TLogRecordField(const TString& fName, const TString& fValue): Value(fValue) {
            if (fName.StartsWith("&")) {
                SignalTagFlag = true;
                Name = fName.substr(1);
            } else if (fName.StartsWith("*")) {
                PriorityFlag = true;
                Name = fName.substr(1);
            } else if (fName == "module") {
                PriorityFlag = true;
                Name = fName;
            } else {
                Name = fName;
            }
            SignalNameFlag = (Name == "unistat_signal");
        }
    }
}
