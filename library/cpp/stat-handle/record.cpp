#include "record.h"

#include <library/cpp/stat-handle/proto/stat.pb.h>

#include <util/generic/ymath.h>
#include <util/stream/printf.h>
#include <util/stream/format.h>
#include <util/stream/str.h>

namespace NStat {
    const std::array<size_t, TDistribution::NUM_COUNTERS> TDistribution::STEPS = {{0, 5, 10, 20, 50, 100}};

    TDistribution& TDistribution::operator+=(const TDistribution& distr) {
        for (size_t i = 0; i < Counters.size(); ++i)
            for (size_t j = 0; j < static_cast<size_t>(EResult::Max); ++j)
                Counters[i].Count[j] += distr.Counters[i].Count[j];

        return *this;
    }

    void TDistribution::Add(const TDuration& duration, EResult t) {
        for (size_t i = 0; i < Counters.size(); ++i) {
            if (duration >= DurationStep(i)) {
                Counters[i][EResult::Total] += 1;
                if (t != EResult::Total && t < EResult::Max)
                    Counters[i][t] += 1;
            } else {
                break;
            }
        }
    }

    size_t TRecord::SortingKey(bool init, bool runtime) const {
        Y_UNUSED(init);
        return runtime ? Timing.Duration.MicroSeconds() : Memory;
    }

    TString HumanReadablePercent(float f, float s) {
        if (fabs(f) <= 1e-9) {
            return "0%";
        } else if (fabs(s) <= 1e-9) {
            return "Inf%";
        }

        TStringStream stream;

        float p = f / s;
        p = floor(p * 100000 + 0.5) / 1000; // round to 3 digits after decimal point
        stream << Prec(p, 3) << "%";

        return stream.Str();
    }

    TString TRecord::PrintInit(const TRecord& total) const {
        TStringStream out;
        out << LeftPad(HumanReadable(InitTime.Duration), 10);
        out << " " << LeftPad(HumanReadablePercent(InitTime.Duration.MicroSeconds(), total.InitTime.Duration.MicroSeconds()), 7);
        out << " " << LeftPad(HumanReadableSize(Memory, SF_BYTES), 10);
        out << " " << LeftPad(HumanReadablePercent(Memory, total.Memory), 7);
        return out.Str();
    }

    TString TRecord::PrintRuntime(const TRecord& total) const {
        TStringStream out;
        out << LeftPad(Timing.Count(EResult::Total), 10);
        out << " " << LeftPad(HumanReadable(Timing.Duration), 10);
        out << " " << LeftPad(HumanReadablePercent(Timing.Duration.MicroSeconds(), total.Timing.Duration.MicroSeconds()), 7);
        out << " " << LeftPad(HumanReadable(Timing.AvgDuration()), 8) << " avg";
        out << " " << LeftPad(HumanReadablePercent(Timing.Count(EResult::Ok), Timing.Count(EResult::Total)), 10) << " ok";
        return out.Str();
    }

    static inline TString TextOrSpaces(bool good, TString text) {
        return good ? text : TString(text.size(), ' ');
    }

    TString TRecord::Print(const TStringBuf& name, bool init, bool runtime, const TRecord* total) const {
        TStringStream out;

        if (!total)
            total = this;

        if (runtime) {
            // print just spaces if there is no runtime part
            out << TextOrSpaces(HasRuntime(), PrintRuntime(*total));
        }

        if (init) {
            if (runtime)
                out << "\t|\t";
            // print just spaces if there is no init part
            out << TextOrSpaces(HasInit(), PrintInit(*total));
        }

        out << "     " << name;
        return out.Str();
    }

    static inline double DoubleMilliSeconds(const TDuration& d) {
        return double(d.MicroSeconds()) / 1000;
    }

    static void CountsToProto(const TDistribution::TCounter& from, TStatProto::TResultCount& to) {
        to.SetTotal(from[EResult::Total]);
        // set only non-zero values
        if (from[EResult::Ok])
            to.SetOk(from[EResult::Ok]);
        if (from[EResult::No])
            to.SetNo(from[EResult::No]);
        if (from[EResult::Error])
            to.SetErrors(from[EResult::Error]);
        if (from[EResult::Cached])
            to.SetCached(from[EResult::Cached]);
    }

    void TRecord::AddToProto(TStatProto& proto, const TString& name) const {
        TStatProto::TRecord& rec = *proto.AddRecords();
        rec.SetName(name);

        if (HasInit()) {
            rec.SetInitTime(DoubleMilliSeconds(InitTime.Duration));
            rec.SetMemory(Memory);
        }

        if (HasRuntime()) {
            rec.SetDuration(DoubleMilliSeconds(Timing.Duration));
            CountsToProto(Timing.Distribution.Total(), *rec.MutableCounts());

            for (size_t i = 0; i < TDistribution::NUM_COUNTERS; ++i) {
                TStatProto::TRecord::TQuantile& q = *rec.AddQuantile();
                q.SetDurationAtLeast(TDistribution::STEPS[i]);
                CountsToProto(Timing.Distribution.Counters[i], *q.MutableCounts());
            }
        }
    }

}
