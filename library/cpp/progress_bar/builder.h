#pragma once

#include "progress.h"
#include "data.h"

#include <util/generic/noncopyable.h>
#include <util/stream/format.h>
#include <util/string/cast.h>
#include <util/string/builder.h>

namespace NProgressBar {
    template <typename T>
    class TBuilder : TNonCopyable {
    public:
        TBuilder(const TString& head, size_t length)
            : Head_(head)
            , Length_(length)
        {
        }

        void Build(const TProgress<T>& progress, TData& out) {
            out.Head = Head_;
            out.Percent = TStringBuilder() << Prec(progress.Current * 100.0 / progress.Total, PREC_POINT_DIGITS, 2) << "%";

            if (out.Percent.size() < Length_) {
                const size_t barLength = Length_ - out.Percent.size();
                TStringBuilder bar;

                for (size_t i = 0; i < barLength; ++i) {
                    if (i * progress.Total <= progress.Current * barLength) {
                        bar << "#";
                    } else {
                        bar << " ";
                    }
                }

                out.Bar = bar;
            }
        }

    private:
        TString Head_;
        const size_t Length_;
    };

}
