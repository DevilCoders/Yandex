#pragma once

#include <library/cpp/progress_bar/progress_bar.h>

#include <util/stream/output.h>

#include "progress.h"

namespace NDoom {


class TStreamProgressCallback {
public:
    TStreamProgressCallback(IOutputStream* stream)
        : Stream_(stream)
    {}

    void Restart(const TString& name) {
        ProgressBar_.Destroy();
        ProgressBar_ = MakeHolder<NProgressBar::TProgressBar<ui64>>(*Stream_, name, [&](){
            NProgressBar::TProgress<ui64> ret;
            ret.Total = TotalProgress_;
            ret.Current = Progress_;
            return ret;
        });
    }

    void Step(ui64 current, ui64 total) {
        TotalProgress_ = total;
        Progress_ = current;
    }

    void Step(const TProgress& progress) {
        Step(progress.Current(), progress.Total());
    }

    void Step() {
        Progress_++;
    }

private:
    IOutputStream* Stream_;
    THolder<NProgressBar::TProgressBar<ui64>> ProgressBar_;
    size_t Progress_ = 0;
    size_t TotalProgress_ = 0;
};


} // namespace NDoom
