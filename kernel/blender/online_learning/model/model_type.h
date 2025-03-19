#pragma once

namespace NBlenderOnlineLearning {
    enum class EModelType {
        LinearSoftmax       /* "linear_softmax" */,
        LinearWinLossBin    /* "linear_winloss_bin" */,
        LinearWinLossMc     /* "linear_winloss_mc" */,
        Unknown             /* "" */
    };
}
