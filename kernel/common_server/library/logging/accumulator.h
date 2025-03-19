#pragma once
#include <kernel/common_server/util/accessor.h>
#include "record/record.h"
#include <library/cpp/logger/priority.h>

namespace NCS {
    namespace NLogging {
        class IEventLogsAccumulator {
        private:
            CS_ACCESS(IEventLogsAccumulator, ui32, RecordsLimit, 1000);
            ui32 RecordsCount = 0;
        protected:
            virtual void DoAddRecord(const TBaseLogRecord& r) = 0;
        public:
            IEventLogsAccumulator(const ui32 recordsLimit = 1000)
                : RecordsLimit(recordsLimit) {

            }
            virtual bool IsLogsAccumulatorEnabled() const {
                return true;
            }
            virtual void AddRecord(const TBaseLogRecord& r) final;
            virtual ELogPriority GetAccumulatorPriority() const {
                return TLOG_INFO;
            }
            virtual TString GetClassName() const = 0;
            virtual ~IEventLogsAccumulator() = default;
        };

        class TBaseLogsAccumulator: public IEventLogsAccumulator {
        private:
            CS_ACCESS(TBaseLogsAccumulator, ELogPriority, EventLogLevel, ELogPriority::TLOG_DEBUG);
            CSA_FLAG(TBaseLogsAccumulator, AccumulatorEnabled, true);
        public:
            virtual bool IsLogsAccumulatorEnabled() const override final {
                return IsAccumulatorEnabled();
            }
            virtual ELogPriority GetAccumulatorPriority() const override final {
                return EventLogLevel;
            }
        };

        class TDefaultLogsAccumulator: public IEventLogsAccumulator {
        private:
            using TBase = IEventLogsAccumulator;
            TVector<TBaseLogRecord> Records;
        protected:
            virtual void DoAddRecord(const TBaseLogRecord& r) override {
                Records.emplace_back(r);
            }
        public:
            using TBase::TBase;
            virtual bool IsLogsAccumulatorEnabled() const override {
                return true;
            }

            virtual TString GetClassName() const override {
                return "default";
            }

            void SerializeXmlReport(NXml::TNode result) const;
            TString GetStringReport() const;
            NJson::TJsonValue GetJsonReport() const;
        };
    }
}
