#pragma once

#include "async_progress_bar.h"
#include "simple_progress_bar.h"

namespace NProgressBar {
    template <typename T>
    using TProgressBar = TAsyncProgressBar<T, TSimpleProgressBar<T>>;
}
