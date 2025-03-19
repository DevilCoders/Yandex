#include "events.h"

#include <kernel/common_server/library/unistat/cache.h>
#include <kernel/common_server/util/object_counter/object_counter.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/xml/document/node-attr.h>

#include <util/datetime/base.h>
#include <util/digest/fnv.h>
#include <util/stream/str.h>
#include <util/stream/zlib.h>
#include <util/system/hostname.h>
#include <util/system/tls.h>
#include <util/string/type.h>
#include <util/system/rwlock.h>

namespace {
    Y_STATIC_THREAD(TVector<NCS::NLogging::IEventLogsAccumulator*>) CurrentAccumulators;
}

template <>
void Out<TFLEventLog::TLogWriterContext>(IOutputStream& out, const TFLEventLog::TLogWriterContext& value) {
    out << value.SerializeToString();
}

namespace {
    TRWMutex LogsNotifierMutex;
    const NCS::NLogging::ILogsAlertsNotifier* AlertNotifier = nullptr;
}

namespace NCS {
    namespace NLogging {
        ELogRecordFormat TEventLog::Format = ELogRecordFormat::TSKV;

        void TEventLog::InitAlertsNotifier(const ILogsAlertsNotifier* notifier) {
            TWriteGuard rg(LogsNotifierMutex);
            AlertNotifier = notifier;
        }

        void TEventLog::DropAlertsNotifier() {
            TWriteGuard rg(LogsNotifierMutex);
            AlertNotifier = nullptr;
        }

        class TEventLogWriterAgent: public IObjectInQueue, public NCSUtil::TObjectCounter<TEventLogWriterAgent> {
        private:
            TBaseLogRecord Record;
            const bool PriorityCheck = false;
        public:
            TEventLogWriterAgent(TBaseLogRecord&& r, const bool priorityCheck)
                : Record(std::move(r))
                , PriorityCheck(priorityCheck) {

            };

            void Process(void*) {
                auto& r = Record;
                if (r.GetIsAlert() && !!AlertNotifier) {
                    TReadGuard rg(LogsNotifierMutex);
                    if (!!AlertNotifier) {
                        AlertNotifier->LoggingNotify(r.GetAlertNotifierId(), r);
                    }
                }
                r.DoSignal();
                TCSSignals::Signal("event_log")("priority", r.GetPriority());
                if (!r.GetAccumulatorClassName()) {
                    if (PriorityCheck) {
                        if (TLoggerOperator<TEventLog>::Usage()) {
                            TLoggerOperator<TEventLog>::Log() << r.GetPriority() << r.SerializeToString(TEventLog::GetLogFormat()) << Endl;
                        } else {
                            SINGLETON_CHECKED_GENERIC_LOG(TGlobalLog, TRTYLogPreprocessor, r.GetPriority(), ToString(r.GetPriority())) << r.SerializeToString(TEventLog::GetLogFormat()) << Endl;
                        }
                    }
                    for (auto&& i : CurrentAccumulators.Get()) {
                        if (r.GetPriority() <= i->GetAccumulatorPriority()) {
                            i->AddRecord(r);
                        }
                    }
                } else {
                    for (auto&& i : CurrentAccumulators.Get()) {
                        if (i->GetClassName() == r.GetAccumulatorClassName()) {
                            if (r.GetPriority() <= i->GetAccumulatorPriority()) {
                                i->AddRecord(r);
                            }
                        }
                    }
                }
            }
        };

        void TEventLog::LogWrite(TBaseLogRecord&& r) {
            auto log = TLoggerOperator<TEventLog>::Get();
            if (!log) {
                Cerr << "LogWrite: " << r.SerializeToString() << Endl;
                return;
            }
            const bool priorityCheck = !log->LogPriorityLimit || *log->LogPriorityLimit >= r.GetPriority();
            if (CurrentAccumulators.Get().empty()) {
                if (!priorityCheck && !r.IsSignal()) {
                    return;
                }
                if (AtomicGet(log->ThreadPoolActive)) {
                    if (!log->ThreadPool.AddAndOwn(MakeHolder<TEventLogWriterAgent>(std::move(r), priorityCheck))) {
                        TCSSignals::Signal("event_log")("priority", "QUEUE_EXCEEDED");
                        if (log->SkipOnExceedWriteQueue) {
                            return;
                        }
                    } else {
                        return;
                    }
                }
            }
            TEventLogWriterAgent(std::move(r), priorityCheck).Process(nullptr);
        }

        bool TEventLog::WaitQueueEmpty(const TInstant deadline) {
            auto log = TLoggerOperator<TEventLog>::Get();
            if (!log) {
                Cerr << "WaitQueueEmpty: Log not initialized" << Endl;
                return false;
            }
            while (AtomicGet(log->ThreadPoolActive) && TEventLogWriterAgent::ObjectCount()) {
                Sleep(TDuration::Seconds(1));
                if (deadline < Now()) {
                    return false;
                }
            }
            return true;
        }

        TEventLog::TLogWriterContext TEventLog::Log(TBaseLogRecord&& r) {
            return TLogWriterContext(std::move(r));
        }

        TEventLog::TLogWriterContext TEventLog::Log(const TBaseLogRecord& r) {
            return TLogWriterContext(r);
        }

        TEventLog::TLogWriterContext TEventLog::Log(const TString& eventInfo, const ELogPriority priority) {
            return TLogWriterContext(TFLRecords::Record(eventInfo, priority));
        }

        TEventLog::TLogWriterContext TEventLog::Debug(const TString& eventInfo) {
            return Log(eventInfo, TLOG_DEBUG);
        }

        TEventLog::TLogWriterContext TEventLog::Lowest(const TString& eventInfo) {
            return Log(eventInfo, TLOG_RESOURCES);
        }

        TEventLog::TLogWriterContext TEventLog::Info(const TString& eventInfo) {
            return Log(eventInfo, TLOG_INFO);
        }

        TEventLog::TLogWriterContext TEventLog::Notice(const TString& eventInfo) {
            return Log(eventInfo, TLOG_NOTICE);
        }

        TEventLog::TLogWriterContext TEventLog::Warning(const TString& eventInfo) {
            return Log(eventInfo, TLOG_WARNING);
        }

        TEventLog::TLogWriterContext TEventLog::Error(const TString& eventInfo) {
            return Log(eventInfo, TLOG_ERR);
        }

        TEventLog::TLogWriterContext TEventLog::Critical(const TString& eventInfo) {
            return Log(eventInfo, TLOG_CRIT);
        }

        TEventLog::TLogWriterContext TEventLog::Signal(const TString& signalName, const double value) {
            return TLogWriterContext(TFLRecords::Record().Signal(signalName, value));
        }

        TEventLog::TLogWriterContext TEventLog::LTSignal(const TString& signalName, const double value) {
            return TLogWriterContext(TFLRecords::Record().LTSignal(signalName, value));
        }

        TEventLog::TLogWriterContext TEventLog::LSignal(const TString& signalName, const double value) {
            return TLogWriterContext(TFLRecords::Record().LSignal(signalName, value));
        }

        TEventLog::TLogWriterContext TEventLog::JustSignal(const TString& signalName, const double value) {
            return TLogWriterContext(TFLRecords::Record("", ELogPriority::TLOG_RESOURCES).Signal(signalName, value));
        }

        TEventLog::TLogWriterContext TEventLog::JustLSignal(const TString& signalName, const double value) {
            return TLogWriterContext(TFLRecords::Record("", ELogPriority::TLOG_RESOURCES).LSignal(signalName, value));
        }

        TEventLog::TLogWriterContext TEventLog::JustLTSignal(const TString& signalName, const double value) {
            return TLogWriterContext(TFLRecords::Record("", ELogPriority::TLOG_RESOURCES).LTSignal(signalName, value));
        }

        TEventLog::TLogWriterContext TEventLog::Alert(const TString& alertText, const TString& alertNotiferId) {
            return TLogWriterContext(TFLRecords::Record(alertText, ELogPriority::TLOG_ALERT).Alert(alertNotiferId));
        }

        TEventLog::TLogWriterContext TEventLog::Log() {
            return TLogWriterContext(TFLRecords::Record());
        }

        TEventLog::TLogWriterContext TEventLog::ModuleLog(const TString& moduleId, const ELogPriority priority) {
            return TLogWriterContext(TFLRecords::Record())("module", moduleId).Priority(priority);
        }

        TEventLog::TContextGuard::TContextGuard(IEventLogsAccumulator* value)
            : ContainedAccumulator(value) {
            if (!value) {
                return;
            }
            if (value->IsLogsAccumulatorEnabled()) {
                CurrentAccumulators.Get().emplace_back(ContainedAccumulator);
            }
        }

        TEventLog::TContextGuard::TContextGuard(const ui32 eventsLimit) {
            InternalAccumulator = MakeAtomicShared<TDefaultLogsAccumulator>(eventsLimit);
            ContainedAccumulator = InternalAccumulator.Get();
            CurrentAccumulators.Get().emplace_back(ContainedAccumulator);
        }

        void TEventLog::TContextGuard::SerializeXmlReport(NXml::TNode result) const {
            if (!!InternalAccumulator) {
                InternalAccumulator->SerializeXmlReport(result);
            }
        }

        TString TEventLog::TContextGuard::GetStringReport() const {
            if (!!InternalAccumulator) {
                return InternalAccumulator->GetStringReport();
            }
            return "";
        }

        NJson::TJsonValue TEventLog::TContextGuard::GetJsonReport() const {
            if (!!InternalAccumulator) {
                return InternalAccumulator->GetJsonReport();
            }
            return NJson::JSON_ARRAY;
        }

        TEventLog::TContextGuard::~TContextGuard() {
            for (auto it = CurrentAccumulators.Get().begin(); it != CurrentAccumulators.Get().end(); ++it) {
                if (*it == ContainedAccumulator) {
                    *it = CurrentAccumulators.Get().back();
                    CurrentAccumulators.Get().pop_back();
                    break;
                }
            }
        }

        TEventLog::TLogWriterContextImpl::TLogWriterContextImpl(TBaseLogRecord&& r)
            : TBase(std::move(r)) {
            Guard = MakeAtomicShared<TWriteLogGuard>();
        }

        TEventLog::TLogWriterContextImpl::TLogWriterContextImpl(const TBaseLogRecord& r)
            : TBase(r) {
            Guard = MakeAtomicShared<TWriteLogGuard>();
        }

        TEventLog::TLogWriterContextImpl::~TLogWriterContextImpl() {
            PrepareFields();
            Guard->SetCurrentRecord(std::move(*this));
        }

        void TEventLog::TWriteLogGuard::SetCurrentRecord(TBaseLogRecord&& currentRecord) {
            CurrentRecord = std::move(currentRecord);
        }

        TEventLog::TWriteLogGuard::~TWriteLogGuard() {
            LogWrite(std::move(CurrentRecord));
        }

        TEventLog::TStartStopGuard::TStartStopGuard(const TString& eventInfo, const bool isForRequestOnly)
            : EventInfo(eventInfo)
            , IsForRequestOnly(isForRequestOnly) {
            if (IsForRequestOnly) {
                TEventLog::Notice("event_started")("event", EventInfo).SetAccumulatorClassName("request_event_log");
            } else {
                TEventLog::Info("event_started")("event", EventInfo);
            }
        }

        TEventLog::TStartStopGuard::~TStartStopGuard() {
            if (IsForRequestOnly) {
                TEventLog::Notice("event_finished")("event", EventInfo).SetAccumulatorClassName("request_event_log");
            } else {
                TEventLog::Info("event_finished")("event", EventInfo);
            }
        }

    }
}
