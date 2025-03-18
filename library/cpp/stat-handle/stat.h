#pragma once

#include "record.h"
#include "mem.h"

#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

#include <util/memory/pool.h>
#include <util/system/mutex.h>

#include <functional>

namespace NStat {
    class TStatProto; // protobuf serialization

    class IRecordWalker {
    public:
        // Should enumerate all contained records with their names
        using TWalkerFunc = std::function<void(const TRecord&, const TString&)>;
        virtual void WalkRecords(const TWalkerFunc& f) const = 0;

        virtual ~IRecordWalker() {
        }
    };

    // Helper class: raw records pool, without any particular indexing/ordering
    class TRecordStorage {
    public:
        TRecordStorage();
        ~TRecordStorage();

        // Range-for iteration
        TVector<TRecord*>::const_iterator begin() const {
            return Records.begin();
        }

        TVector<TRecord*>::const_iterator end() const {
            return Records.end();
        }

        TRecord* MakeNewRecord();

    private:
        void DeleteRecord(TRecord* item);

    private:
        TMemoryPool Pool;          // owns all records
        TVector<TRecord*> Records; // all records in order of insertion
    };

    // Basic interface without indexing policy.
    class TStatBase: protected  TRecordStorage, protected IRecordWalker {
    public:
        TStatBase(bool memUsage = false);
        virtual ~TStatBase();

        bool TracingMemoryUsage() const {
            return TraceMemUsage;
        }

        TDuration Age() const;           // time during which the stat is collected
        TDuration TotalDuration() const; // sum of all records durations

        // serialization
        virtual void ToProto(TStatProto& proto) const;
        //! A convenience method that works exactly the same as ToProto(NStat::TStatProto& proto);
        NStat::TStatProto ToProto() const;
        virtual TString ToJson() const;

        TString PrintInit(const TString& title, size_t top = 0) const;
        TString PrintRuntime(const TString& title, size_t top = 0) const;
        TString PrintAll(const TString& title, size_t top = 0) const;

    protected:
        virtual TString DoPrint(const TString& title, size_t top, bool init, bool runtime) const;

    private:
        TRecord TotalRecord() const;

    private:
        bool TraceMemUsage = false;
        TInstant Started;
    };

    // Basic stat interface with records indexed by string ids (e.g. rule names).
    class TStat: public TStatBase {
    public:
        TStat(bool memUsage = false)
            : TStatBase(memUsage)
        {
        }

        TStat& operator+=(const TStat& stat) {
            for (auto it : stat.Index)
                AppendRecord(it.first, *(it.second));
            return *this;
        }

        template <typename TRecordId>
        void AppendRecord(TRecordId id, const TRecord& record) {
            GetOrMakeRecord(id) += record;
        }

        TRecord& GetOrMakeRecord(const TString& id) {
            auto ins = Index.insert({id, nullptr});
            if (ins.second)
                ins.first->second = MakeNewRecord();

            return *(ins.first->second);
        }

        // more efficient for top-level request stats accumulation,
        // assumes most ids are already known
        TRecord& GetOrMakeRecord(const TStringBuf& id) {
            auto it = Index.find(id);
            return it != Index.end() ? *(it->second) : GetOrMakeRecord(TString(id));
        }

    protected:
        // Implements IRecordWalker
        virtual void WalkRecords(const TWalkerFunc& func) const override {
            for (auto it : Index)
                func(*it.second, it.first);
        }

    private:
        THashMap<TString, TRecord*> Index;
    };

    // Part of stat indexed with enum values [0, MaxCount).
    // This class can be added to TStat as base class
    // in order to use faster indexing scheme.
    // Uses global ToString(TEnum) to convert enum value into string name.
    // See TWizardStat as an example of usage.
    template <typename TEnum, size_t MaxCount>
    class TRecordEnumIndex {
    public:
        TRecordEnumIndex(TRecordStorage& parent)
            : Parent(parent)
        {
            Index.resize(MaxCount, nullptr);
        }

        TRecordEnumIndex& operator+=(const TRecordEnumIndex& stat) {
            for (size_t i = 0; i < stat.Index.size(); ++i)
                if (stat.Index[i])
                    GetOrMakeRecord(TEnum(i)) += *stat.Index[i];

            return *this;
        }

        TRecord& GetOrMakeRecord(TEnum id) {
            if (Y_UNLIKELY(size_t(id) >= MaxCount))
                ythrow yexception() << "Invalid enum id: " << size_t(id);
            TRecord*& ret = Index[id];
            if (!ret)
                ret = Parent.MakeNewRecord();
            return *ret;
        }

        // instead of IRecordWalker implements its own walker
        using TWalkerEnumFunc = std::function<void(const TRecord&, TEnum)>;
        void WalkRecords(const TWalkerEnumFunc& func) const {
            for (size_t i = 0; i < Index.size(); ++i) {
                if (Index[i])
                    func(*Index[i], TEnum(i));
            }
        }

    private:
        TRecordStorage& Parent; // parent record storage
        TVector<TRecord*> Index;
    };

    /** A RAII-timer that tracks execution time of smth (of its enclosing stack frame actually).
 * Usage:
@code
{
    TTimer timer([](const TTimeInfo& t){
        Cerr << "successful requests: " << t.Count(EResult::Ok) << "\n";
        Cerr << "     error requests: " << t.Count(EResult::Error) << "\n";
    });
    // payload
}
@endcode
    The timer stores only one request, at its destructor. The request type is useful
    for later merging the histograms from multiple requests.
    To set the request type, use:
    - SetResult: true for the 'Yes' response, false for the 'No' response
    - SetError: if the request was not fullfilled, no response generated
    - SetCached: the response was cached (be it yes or no).

    By default the response type is invalid, and the result will be added only
    to the TOTAL response count.
    @see record.h for the definition of EResultType.
 */
    class TTimer {
    public:
        using TOnStop = std::function<void(const TTimeInfo&)>;

        TTimer(TOnStop&& onStop)
            : OnStop(std::move(onStop))
        {
            Timing.StartedNow();
        }

        template <typename TTStat, typename TRecordId>
        TTimer(TTStat* stat, TRecordId id)
            : TTimer([=](const TTimeInfo& t)
        {
                if (stat)
                    stat->GetOrMakeRecord(id).Timing += t;
            }) {
        }

        ~TTimer() {
            Timing.FinishedNow(Result);
            OnStop(Timing);
        }

        void SetResult(bool success) {
            Result = success ? EResult::Ok : EResult::No;
        }

        void SetError() {
            Result = EResult::Error;
        }

        void SetCached() {
            Result = EResult::Cached;
        }

    private:
        TOnStop OnStop;
        TTimeInfo Timing;
        EResult Result = EResult::Max;
    };

    // Specialized RAII counter for initialization step,
    // measures consumed time and memory.
    // Requires mutex for memory usage tracing.
    class TInit {
    public:
        template <typename TTStat, typename TRecordId>
        TInit(TTStat* stat, TRecordId id, TMutex* mutex = nullptr) {
            if (stat) {
                TRecord& rec = stat->GetOrMakeRecord(id);
                if (stat->TracingMemoryUsage())
                    StartMemTrace(rec, mutex);

                Timer.Reset(new TTimer([&rec](const TTimeInfo& t) {
                    rec.InitTime += t;
                }));
            }
        }

    private:
        void StartMemTrace(TRecord& rec, TMutex* mutex) {
            MemCounter.Reset(new TMemCounter(rec.Memory, mutex));
        }

        THolder<TMemCounter> MemCounter;
        THolder<TTimer> Timer;
    };

}
