#include "stat.h"
#include <library/cpp/stat-handle/proto/stat.pb.h>

#include <library/cpp/protobuf/json/proto2json.h>
#include <library/cpp/protobuf/util/repeated_field_utils.h>

#include <util/generic/map.h>
#include <util/stream/format.h>

namespace NStat {
    TRecordStorage::TRecordStorage()
        : Pool(sizeof(TRecord) * 256, TMemoryPool::TLinearGrow::Instance())
    {
        Records.reserve(256);
    }

    TRecordStorage::~TRecordStorage() {
        // explicitly destruct all items in the Pool
        for (size_t i = 0; i < Records.size(); ++i)
            DeleteRecord(Records[i]);
    }

    TRecord* TRecordStorage::MakeNewRecord() {
        TRecord* rec = Pool.New<TRecord>();
        Records.push_back(rec);
        return rec;
    }

    void TRecordStorage::DeleteRecord(TRecord* item) {
        if (item)
            item->~TRecord();
    }

    TStatBase::TStatBase(bool traceMem)
        : TraceMemUsage(traceMem)
        , Started(Now())
    {
    }

    TStatBase::~TStatBase() {
    }

    TRecord TStatBase::TotalRecord() const {
        TRecord total;
        for (const TRecord* r : *this)
            total += *r;

        return total;
    }

    TDuration TStatBase::Age() const {
        return Now() - Started;
    }

    TDuration TStatBase::TotalDuration() const {
        TDuration total;
        for (const TRecord* r : *this)
            total += r->Timing.Duration;

        return total;
    }

    static const TString TOTAL_NAME("Total");

    void TStatBase::ToProto(TStatProto& proto) const {
        WalkRecords([&proto](const TRecord& rec, const TString& id) {
            rec.AddToProto(proto, id);
        });

        TotalRecord().AddToProto(proto, TOTAL_NAME);
        // remove just added Record and put it into Total
        THolder<TStatProto::TRecord> total(proto.MutableRecords()->ReleaseLast());
        proto.MutableTotal()->Swap(total.Get());

        proto.SetAge(Age().MilliSeconds());
    }

    NStat::TStatProto TStatBase::ToProto() const {
        NStat::TStatProto ret;
        ToProto(ret);
        return ret;
    }

    TString TStatBase::ToJson() const {
        // This method is copy-pasted into search/wizard/common/stat.h to ensure its thread-safety.
        // If you irrecoverably fix this method here, fix it also there, please.
        TStatProto proto;
        ToProto(proto);
        TString out;
        NProtobufJson::Proto2Json(proto, out);
        return out;
    }

    TString TStatBase::DoPrint(const TString& title, size_t top, bool init, bool runtime) const {
        // collect and sort records by their SortingKey()
        TMultiMap<size_t, std::pair<TString, const TRecord*>> durations;
        WalkRecords([&durations, init, runtime](const TRecord& rec, const TString& id) {
            using std::make_pair;
            durations.insert(make_pair(rec.SortingKey(init, runtime), make_pair(id, &rec)));
        });

        size_t printed = 0;
        if (top == 0)
            top = Max<size_t>();

        TStringStream out;

        out << title;
        out << " (for last " << HumanReadable(Age()) << "):\n";

        TRecord total = TotalRecord();

        size_t maxline = 0;
        for (auto it = durations.rbegin(); it != durations.rend() && printed < top; ++it) {
            const TRecord& r = *it->second.second;
            if (r.IsPrintable(init, runtime)) {
                ++printed;
                TString line = r.Print(it->second.first, init, runtime, &total);
                out << line << "\n";
                if (maxline < line.size())
                    maxline = line.size();
            }
        }

        if (printed < durations.size())
            out << LeftPad("...  ", 10) << "\n";
        out << TString(maxline, '=') << "\n";
        out << total.Print("total", init, runtime, nullptr) << "\n";

        return out.Str();
    }

    TString TStatBase::PrintInit(const TString& title, size_t top) const {
        return DoPrint(title, top, true, false);
    }

    TString TStatBase::PrintRuntime(const TString& title, size_t top) const {
        return DoPrint(title, top, false, true);
    }

    TString TStatBase::PrintAll(const TString& title, size_t top) const {
        return DoPrint(title, top, true, true);
    }

}
