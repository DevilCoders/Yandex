#include "output_utime.h"

#include <util/datetime/base.h>
#include <util/generic/algorithm.h>
#include <util/stream/output.h>

namespace NSnippets {

    TUTimeOutput::TUTimeOutput(bool idSuffix)
      : IdSuffix(idSuffix)
    {
    }

    void TUTimeOutput::Process(const TJob& job) {
        Cout << job.UTime.MicroSeconds();
        if (IdSuffix) {
            Cout << " " << job.ContextData.GetId();
        }
        Cout << Endl;
    }


    void TUTimeTableOutput::Process(const TJob& job) {
        Times.push_back(job.UTime.MicroSeconds());
    }

    void TUTimeTableOutput::Complete() {
        if (Times.empty()) {
            return;
        }
        Sort(Times.begin(), Times.end());
        for (int i = 0; i <= 20; ++i) {
            if (i == 20) {
                Cout << "99%: " << Times[(Times.size() - 1) * 99 / 100] << Endl;
            }
            Cout << 5 * i << "%: " << Times[(Times.size() - 1) * i / 20] << Endl;
        }
    }

} //namespace NSnippets
