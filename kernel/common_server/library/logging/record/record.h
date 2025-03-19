#pragma once
#include "tskv.h"
#include <library/cpp/charset/ci_string.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/string_utils/base64/base64.h>
#include <kernel/common_server/util/accessor.h>
#include <kernel/common_server/library/unistat/cache.h>
#include <util/digest/fnv.h>
#include <util/generic/guid.h>
#include <util/system/hostname.h>
#include <util/generic/set.h>
#include <util/string/join.h>

namespace NXml {
    class TNode;
}
namespace NCS {
    namespace NLogging {

        enum class ELogRecordFormat {
            TSKV,
            Json,
            HR
        };
        enum class ESignalType {
            CounterWithTimeout,
            Counter,
            Increment,
            NoSignal
        };

        class TEventLogSettings {
        public:
            static ui32 LineLengthLimit;
        };

        class TLogRecordField {
        private:
            CSA_READONLY_DEF(TString, Name);
            CSA_READONLY_DEF(TString, Value);
            CSA_READONLY_FLAG(SignalTag, false);
            CSA_READONLY_FLAG(Priority, false);
            CSA_READONLY_FLAG(SignalName, false);
        public:
            TLogRecordField(const TString& fName, const TString& fValue);

            bool operator<(const TLogRecordField& item) const {
                return Name < item.Name;
            }
        };

        class TBaseLogRecord {
        private:
            CSA_READONLY_MUTABLE_DEF(TVector<TLogRecordField>, Fields);
            CSA_READONLY(bool, IsAlert, false);
            CSA_READONLY(ESignalType, SignalType, ESignalType::NoSignal);
            CSA_READONLY(double, Value, 0);
            CSA_DEFAULT(TBaseLogRecord, TString, AlertNotifierId);
            CSA_DEFAULT(TBaseLogRecord, TString, AccumulatorClassName);
            CS_ACCESS(TBaseLogRecord, bool, IsContext, false);
            CS_ACCESS(TBaseLogRecord, ELogPriority, Priority, TLOG_INFO);
            mutable ui32 SortedCount = 0;
            mutable i32 WrotePriority = -1;
        protected:
            TBaseLogRecord& Add(const TString& key, const NJson::TMapBuilder& value, const ui32 logLineLimit = TEventLogSettings::LineLengthLimit);
            TBaseLogRecord& Merge(const TBaseLogRecord& record);
            bool IsAvailableField(const TLogRecordField& f) const;
            void PrepareFields() const;
        public:
            bool IsSignal() const {
                return SignalType != ESignalType::NoSignal;
            }

            TBaseLogRecord& Lowest(const TString& text = Default<TString>()) noexcept;
            TBaseLogRecord& Debug(const TString& text = Default<TString>()) noexcept;
            TBaseLogRecord& Info(const TString& text = Default<TString>()) noexcept;
            TBaseLogRecord& Notice(const TString& text = Default<TString>()) noexcept;
            TBaseLogRecord& Warning(const TString& text = Default<TString>()) noexcept;
            TBaseLogRecord& Error(const TString& text = Default<TString>()) noexcept;
            TBaseLogRecord& Critical(const TString& text = Default<TString>()) noexcept;
            TBaseLogRecord& AddFields(const TVector<TLogRecordField>& fields) {
                Fields.insert(Fields.end(), fields.begin(), fields.end());
                return *this;
            }

            template<class TContainer>
            TBaseLogRecord& AddSignalTags(const TContainer& tags) {
                for (const auto& tag : tags) {
                    Add("&" + tag.first, tag.second);
                }
                return *this;
            }
        public:
            template <class TValue>
            TBaseLogRecord& Add(const TString& key, const TValue& value, const ui32 logLineLimit = TEventLogSettings::LineLengthLimit) {
                Add(key, ::ToString(value), logLineLimit);
                return *this;
            }

            template <class TValue>
            TBaseLogRecord& Add(const TString& key, const TVector<TValue>& values, const ui32 logLineLimit = TEventLogSettings::LineLengthLimit) {
                Add(key, JoinSeq(",", values), logLineLimit);
                return *this;
            }

            template <class TValue>
            TBaseLogRecord& Add(const TString& key, const TSet<TValue>& values, const ui32 logLineLimit = TEventLogSettings::LineLengthLimit) {
                Add(key, JoinSeq(",", values), logLineLimit);
                return *this;
            }

            TBaseLogRecord& Add(const TString& key, const TString& value, const ui32 logLineLimit = TEventLogSettings::LineLengthLimit) noexcept;
            TBaseLogRecord& AddEscaped(const TString& key, const TString& value, const ui32 logLineLimit = TEventLogSettings::LineLengthLimit);

            void InitBaseRecord();

            TString ToStringLocalWithMs(const TInstant& instant) const;

            void DoSignal() const;
            TBaseLogRecord& SignalDetails(const TString& signalName, const double value, const ESignalType signalType);
            TBaseLogRecord& Signal(const TString& signalName = "", const double value = 1);
            TBaseLogRecord& LTSignal(const TString& signalName = "", const double value = 0);
            TBaseLogRecord& LSignal(const TString& signalName = "", const double value = 0);
            TBaseLogRecord& Alert(const TString& alertNotifierId = "");

            TBaseLogRecord& RequestEventLog(const TString& requestEvLogCollector = "request_event_log") {
                AccumulatorClassName = requestEvLogCollector;
                return *this;
            }

            TBaseLogRecord& Source(const TString& id) {
                return Add("source", id);
            }

            TBaseLogRecord& Module(const TString& id) {
                return Add("module", id);
            }

            TBaseLogRecord& Method(const TString& methodId) {
                return Add("*method", methodId);
            }

            TBaseLogRecord& Trace(const bool escape = false);

            TVector<TLogRecordField>::const_iterator begin() const {
                return Fields.begin();
            }

            TVector<TLogRecordField>::const_iterator end() const {
                return Fields.end();
            }

            NJson::TJsonValue SerializeToJson() const;
            void SerializeToXml(NXml::TNode result) const;

            template <class TActor>
            void ScanFields(const TActor& actor) const {
                PrepareFields();
                TString predKey;
                TString currentValue;
                bool currentPriorityField = false;
                for (auto&& i : Fields) {
                    if (!IsAvailableField(i)) {
                        continue;
                    }
                    if (predKey == i.GetName()) {
                        if (currentPriorityField && !i.IsPriority()) {
                            continue;
                        }
                    } else {
                        if (!!predKey) {
                            try {
                                actor(predKey, currentValue);
                            } catch (...) {
                                ERROR_LOG << "EventLog::Log " << predKey << " / " << currentValue << " triggered an exception: " << CurrentExceptionMessage() << Endl;
                            }
                        }
                        predKey = i.GetName();
                    }
                    currentValue = i.GetValue();
                    currentPriorityField = i.IsPriority();
                }
                if (!!predKey) {
                    try {
                        actor(predKey, currentValue);
                    } catch (...) {
                        ERROR_LOG << "EventLog::Log " << predKey << " / " << currentValue << " triggered an exception: " << CurrentExceptionMessage() << Endl;
                    }
                }
            }

            TString SerializeToString(const ELogRecordFormat format = ELogRecordFormat::TSKV) const;
        };

        template <class TBaseRecord>
        class TLogRecordOperator: public TBaseRecord {
        protected:
            using TBaseRecord::Add;
        public:
            using TBaseRecord::TBaseRecord;
            TLogRecordOperator(const TBaseRecord& r)
                : TBaseRecord(r) {

            }

            TLogRecordOperator& operator()(const TBaseRecord& record) {
                TBaseRecord::Merge(record);
                return *this;
            }

            template <class TValue>
            TLogRecordOperator& operator()(const TString& key, const TValue& value, const ui32 lineLengthLimit = TEventLogSettings::LineLengthLimit) {
                TBaseRecord::Add(key, value, lineLengthLimit);
                return *this;
            }

            template <class TValue>
            TLogRecordOperator& operator()(const TMap<TString, TValue>& events, const ui32 lineLengthLimit = TEventLogSettings::LineLengthLimit) {
                for (auto&& i : events) {
                    TBaseRecord::Add(i.first, ::ToString(i.second), lineLengthLimit);
                }
                return *this;
            }

            template <class TValue>
            TLogRecordOperator& operator()(const TString& key, const TVector<TValue>& values, const ui32 lineLengthLimit = TEventLogSettings::LineLengthLimit) {
                TBaseRecord::Add(key, values, lineLengthLimit);
                return *this;
            }

            template <class TValue>
            TLogRecordOperator& operator()(const TString& key, const TSet<TValue>& values, const ui32 lineLengthLimit = TEventLogSettings::LineLengthLimit) {
                TBaseRecord::Add(key, values, lineLengthLimit);
                return *this;
            }
        };

        class TLogRecord: public TLogRecordOperator<TBaseLogRecord> {

        };

        class TLogThreadContext: public TLogRecord {
        private:
            using TBase = TLogRecord;
            CSA_READONLY(TString, ContextId, TGUID::Create().AsGuidString());
        public:
            TLogThreadContext() {
                SetIsContext(true);
            }
            ~TLogThreadContext();

            class TGuard {
            private:
                TAtomicSharedPtr<TLogThreadContext> Context;
            public:
                TGuard(TAtomicSharedPtr<TLogThreadContext> context)
                    : Context(std::move(context)) {

                }

                TGuard& Method(const TString& id) {
                    Context->Method(id);
                    return *this;
                }

                TGuard& SignalId(const TString& id) {
                    (*Context)("*unistat_signal", id);
                    return *this;
                }

                TGuard& Source(const TString& id) {
                    Context->Source(id);
                    return *this;
                }

                TGuard& Module(const TString& id) {
                    Context->Module(id);
                    return *this;
                }

                template <class TValue>
                TGuard& operator()(const TMap<TString, TValue>& events) {
                    (*Context)(events);
                    return *this;
                }

                TGuard& operator()(const TBaseLogRecord& record) {
                    (*Context)(record);
                    return *this;
                }

                template <class T>
                TGuard& operator()(const TString& key, const T& value) {
                    (*Context)(key, value);
                    return *this;
                }

                template<class TContainer>
                TGuard& AddSignalTags(const TContainer& tags) {
                    Context->AddSignalTags(tags);
                    return *this;
                }
            };
        };

        class TLogRecordConstructor {
        public:
            static TLogThreadContext::TGuard StartContext();
            static TLogRecord Record();
            static TLogRecord& FillContext(TLogRecord& record);
            static TLogRecord Record(const TString& eventInfo, const ELogPriority priority = ELogPriority::TLOG_INFO);
            static TLogRecord BaseRecord();
            static TSignalTagsSet CollectSignalTags();
        };
    }
}

using TFLRecords = NCS::NLogging::TLogRecordConstructor;
