#pragma once

#include "request_params.h"
#include "stat.h"

#include <antirobot/idl/antirobot.ev.pb.h>
#include <antirobot/idl/daemon_log.pb.h>

#include <library/cpp/logger/log.h>
#include <library/cpp/logger/null.h>
#include <library/cpp/logger/thread.h>

#include <logbroker/unified_agent/client/cpp/logger/backend.h>


namespace NAntiRobot {
    struct TRequestContext;

    class TUnifiedAgentLogBackend : public TLogBackend {
    public:
        explicit TUnifiedAgentLogBackend(const TString& unifiedAgentUri, const TString& logType, size_t queueLen = 100000);
        
        void WriteData(const TLogRecord& rec) override {
            Counters.Inc(ECounter::Records);
            if (UnifiedAgentBackend) {
                UnifiedAgentBackend->WriteData(rec);
            }
        }

        void ReopenLog() override {
            if (UnifiedAgentBackend) {
                UnifiedAgentBackend->ReopenLog();
            }
        }

        template <typename Record>
        void WriteLogRecord(const Record& rec) {
            if (!UnifiedAgentBackend) {
                return;
            }
            const auto serializedRecord = rec.SerializeAsString();
            WriteData(TLogRecord(LOG_DEF_PRIORITY, serializedRecord.Data(), serializedRecord.Size()));
        }

        void PrintStats(TStatsWriter& out) const {
            Counters.Print(out, Prefix);
            if (UnifiedAgentBackend) {
                out.WriteHistogram(Prefix + "queue_size", UnifiedAgentBackend->QueueSize());
            }
        }

        enum class ECounter {
            Records              /* "records" */,
            RecordsOverflow      /* "records_overflow" */,
            Count
        };

    protected:
        THolder<TThreadedLogBackend> UnifiedAgentBackend;
        TString Prefix;
        TCategorizedStats<std::atomic<size_t>, ECounter> Counters;
    };

    class TUnifiedAgentDaemonLog : public TLog {
    public:
        explicit TUnifiedAgentDaemonLog(const TString& unifiedAgentUri);
    };

    template <class T>
    NAntirobotEvClass::TProtoseqRecord CreateEventLogRecord(const T& ev) {
        TBufferOutput buf;
        const ui64 timestamp = Now().MicroSeconds();
        const ui32 messageid = ev.GetDescriptor()->options().GetExtension(message_id);
        const size_t messageSize = ev.ByteSize();
        const auto message = ev.SerializeAsString();
        const ui32 size = sizeof(ui32) + sizeof(timestamp) + sizeof(messageid) + messageSize;

        ::Save(&buf, size);
        ::Save(&buf, timestamp);
        ::Save(&buf, messageid);
        buf.Buffer().Append(message.data(), messageSize);

        NAntirobotEvClass::TProtoseqRecord rec;
        rec.set_event(buf.Buffer().Data(), buf.Buffer().Size());
        rec.set_timestamp(timestamp / 1000);
        return rec;
    }
    
    template <typename T>
    NProtoBuf::RepeatedPtrField<TString> ConvertToProtoStrings(const TVector<T>& src) {
        const auto mappedSrc = MakeMappedRange(src, [] (const T& x) {
            return ToString(x);
        });

        return {mappedSrc.begin(), mappedSrc.end()};
    }

    NProtoBuf::RepeatedField<ui32> ConvertExpHeaderToProto(const TVector<TRequest::TExpInfo>& src);
}
