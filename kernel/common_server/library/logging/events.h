#pragma once

#include <kernel/searchlog/searchlog.h>
#include <kernel/daemon/config/daemon_config.h>

#include <kernel/common_server/library/logging/record/record.h>
#include "accumulator.h"

namespace NCS {
    namespace NLogging {
        class ILogsAlertsNotifier {
        public:
            using TPtr = TAtomicSharedPtr<ILogsAlertsNotifier>;

            virtual ~ILogsAlertsNotifier() = default;
            virtual void LoggingNotify(const TString& notifierId, const TBaseLogRecord& r) const = 0;
        };

        class TEventLog: public TSearchLog {
        private:
            static ELogRecordFormat Format;
            static void LogWrite(TBaseLogRecord&& r);

            class TWriteLogGuard {
            private:
                TBaseLogRecord CurrentRecord;
            public:
                void SetCurrentRecord(TBaseLogRecord&& currentRecord);

                ~TWriteLogGuard();
            };

            class TLogWriterContextImpl: public TBaseLogRecord {
            private:
                using TBase = TBaseLogRecord;
                TAtomicSharedPtr<TWriteLogGuard> Guard;
            public:
                explicit TLogWriterContextImpl(TBaseLogRecord&& r);
                explicit TLogWriterContextImpl(const TBaseLogRecord& r);
                ~TLogWriterContextImpl();
            };
            TAtomic ThreadPoolActive = 0;
            TThreadPool ThreadPool;
            bool SkipOnExceedWriteQueue = true;
            TMaybe<ELogPriority> LogPriorityLimit;
        public:
            static void InitAlertsNotifier(const ILogsAlertsNotifier* notifier);
            static void DropAlertsNotifier();

            ~TEventLog() {
                ThreadPool.Stop();
            }

            void UsePriorityLimit(const ELogPriority limit) {
                LogPriorityLimit = limit;
            }

            void ActivateBackgroundWriting(const ui32 queueSizeLimit, const bool skipOnExceedWriteQueue) {
                AtomicSet(ThreadPoolActive, 1);
                SkipOnExceedWriteQueue = skipOnExceedWriteQueue;
                ThreadPool.Start(1, queueSizeLimit);
            }

            static bool WaitQueueEmpty(const TInstant deadline);

            class TStartStopGuard {
            private:
                CSA_READONLY_DEF(TString, EventInfo);
                CSA_READONLY(bool, IsForRequestOnly, true);
            public:
                TStartStopGuard(const TString& eventInfo, const bool isForRequestOnly);
                ~TStartStopGuard();
            };

            static TAtomicSharedPtr<TStartStopGuard> ReqEventLogGuard(const TString& evInfo) {
                return MakeAtomicShared<TStartStopGuard>(evInfo, true);
            }

            static TAtomicSharedPtr<TStartStopGuard> EvLogGuard(const TString& evInfo) {
                return MakeAtomicShared<TStartStopGuard>(evInfo, false);
            }

            class TContextGuard {
            private:
                TAtomicSharedPtr<TDefaultLogsAccumulator> InternalAccumulator;
                IEventLogsAccumulator* ContainedAccumulator = nullptr;
            public:
                TContextGuard(const ui32 eventsLimit = 1000);
                TContextGuard(IEventLogsAccumulator* value);

                TString GetStringReport() const;
                NJson::TJsonValue GetJsonReport() const;
                void SerializeXmlReport(NXml::TNode result) const;

                ~TContextGuard();
            };

            class TLogWriterContext: public TLogRecordOperator<TLogWriterContextImpl> {
            private:
                using TBase = TLogRecordOperator<TLogWriterContextImpl>;
            public:
                using TBase::TBase;
                TLogWriterContext& operator()(const TBaseLogRecord& record) {
                    AddFields(record.GetFields());
                    return *this;
                }

                template <class TValue>
                TLogWriterContext& operator()(const TString& key, const TValue& value, const ui32 logLineLimit = TEventLogSettings::LineLengthLimit) {
                    TBase::Add(key, value, logLineLimit);
                    return *this;
                }

                template <class TValue>
                TLogWriterContext& operator()(const TMap<TString, TValue>& events, const ui32 logLineLimit = TEventLogSettings::LineLengthLimit) {
                    for (auto&& i : events) {
                        TBase::Add(i.first, ::ToString(i.second), logLineLimit);
                    }
                    return *this;
                }

                TLogWriterContext& Module(const TString& id) {
                    TBase::Module(id);
                    return *this;
                }

                TLogWriterContext& Signal(const TString& signalName = "", const double value = 1) {
                    TBase::Signal(signalName, value);
                    return *this;
                }

                TLogWriterContext& LTSignal(const TString& signalName = "", const double value = 0) {
                    TBase::LTSignal(signalName, value);
                    return *this;
                }

                TLogWriterContext& JustSignal(const TString& signalName = "", const double value = 1) {
                    TBase::Signal(signalName, value).SetPriority(ELogPriority::TLOG_RESOURCES);
                    return *this;
                }

                TLogWriterContext& JustLSignal(const TString& signalName = "", const double value = 1) {
                    TBase::LSignal(signalName, value).SetPriority(ELogPriority::TLOG_RESOURCES);
                    return *this;
                }

                TLogWriterContext& JustLTSignal(const TString& signalName = "", const double value = 0) {
                    TBase::LTSignal(signalName, value).SetPriority(ELogPriority::TLOG_RESOURCES);
                    return *this;
                }

                TLogWriterContext& Method(const TString& id) {
                    TBase::Method(id);
                    return *this;
                }

                TLogWriterContext& Source(const TString& id) {
                    TBase::Source(id);
                    return *this;
                }

                TLogWriterContext& Priority(const ELogPriority priority) {
                    TBase::SetPriority(priority);
                    return *this;
                }
                TLogWriterContext& Debug(const TString& text = Default<TString>()) {
                    TBase::Debug(text);
                    return *this;
                }
                TLogWriterContext& Lowest(const TString& text = Default<TString>()) {
                    TBase::Lowest(text);
                    return *this;
                }
                TLogWriterContext& Info(const TString& text = Default<TString>()) {
                    TBase::Info(text);
                    return *this;
                }
                TLogWriterContext& Notice(const TString& text = Default<TString>()) {
                    TBase::Notice(text);
                    return *this;
                }
                TLogWriterContext& Warning(const TString& text = Default<TString>()) {
                    TBase::Warning(text);
                    return *this;
                }
                TLogWriterContext& Error(const TString& text = Default<TString>()) {
                    TBase::Error(text);
                    return *this;
                }
                TLogWriterContext& Critical(const TString& text = Default<TString>()) {
                    TBase::Critical(text);
                    return *this;
                }
                TLogWriterContext& Alert(const TString& text = Default<TString>()) {
                    TBase::Alert(text);
                    return *this;
                }
            };

            using TSearchLog::TSearchLog;

            static TLogWriterContext Log(TBaseLogRecord&& r);
            static TLogWriterContext Log(const TBaseLogRecord& r);
            static TLogWriterContext Log(const TString& eventInfo, const ELogPriority priority = TLOG_INFO);
            static TLogWriterContext Lowest(const TString& eventInfo = Default<TString>());
            static TLogWriterContext Debug(const TString& eventInfo = Default<TString>());
            static TLogWriterContext Info(const TString& eventInfo = Default<TString>());
            static TLogWriterContext Notice(const TString& eventInfo = Default<TString>());
            static TLogWriterContext Warning(const TString& eventInfo = Default<TString>());
            static TLogWriterContext Error(const TString& eventInfo = Default<TString>());
            static TLogWriterContext Critical(const TString& eventInfo = Default<TString>());
            static TLogWriterContext Signal(const TString& signalName = "", const double value = 1);
            static TLogWriterContext LTSignal(const TString& signalName = "", const double value = 0);
            static TLogWriterContext LSignal(const TString& signalName = "", const double value = 0);
            static TLogWriterContext JustSignal(const TString& signalName = "", const double value = 1);
            static TLogWriterContext JustLSignal(const TString& signalName = "", const double value = 0);
            static TLogWriterContext JustLTSignal(const TString& signalName = "", const double value = 0);
            static TLogWriterContext Alert(const TString& alertText, const TString& alertNotiferId = "");

            /*[[deprecated (Use method with direct priority class mentioning)]]*/
            static TLogWriterContext Log();
            static TLogWriterContext ModuleLog(const TString& moduleId, const ELogPriority priority = TLOG_INFO);

            static ELogRecordFormat GetLogFormat() {
                return Format;
            }

            static void SetFormat(const ELogRecordFormat format) {
                Format = format;
            }
        };
    }
}
using TFLEventLog = NCS::NLogging::TEventLog;
using TFLEventLogGuard = TAtomicSharedPtr<TFLEventLog::TStartStopGuard>;
