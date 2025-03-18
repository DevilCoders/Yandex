#pragma once

#include "progress.h"
#include "data.h"
#include "builder.h"

#include <library/cpp/colorizer/colors.h>

#include <util/stream/output.h>

namespace NProgressBar {
    template <typename T>
    class TSimpleProgressBar : TNonCopyable {
    public:
        explicit TSimpleProgressBar(IOutputStream& outputStream, const TString& head, size_t length = 80)
            : OutputStream_(outputStream)
            , Colors_(NColorizer::AutoColors(OutputStream_))
            , Builder_(head, length)
        {
            OutputStream_ << Colors_.BlueColor() << "==> " << Colors_.OldColor()
                          << head << Endl;
        }

        void Update(const TProgress<T>& progress) {
            Builder_.Build(progress, DataBuffer_);
            if (Colors_.IsTTY()) {
                OutputStream_ << "\r" << DataBuffer_.Bar << " " << DataBuffer_.Percent;
            } else {
                OutputStream_ << DataBuffer_.Percent << Endl;
            }
        }

        ~TSimpleProgressBar() {
            Flush();
        }

        void Flush() {
            if (!Flushed_) {
                OutputStream_ << Endl;
                Flushed_ = true;
            }
        }

    private:
        IOutputStream& OutputStream_;
        NColorizer::TColors& Colors_;
        TData DataBuffer_;
        TBuilder<T> Builder_;
        bool Flushed_ = false;
    };

}
