#pragma once

#include <util/system/types.h>

namespace NDoom {

class TProgress {
public:
    TProgress() {
    }
    TProgress(ui64 current, ui64 total)
        : Current_(current)
        , Total_(total)
    {
    }

    ui64 Current() const {
        return Current_;
    }

    ui64 Total() const {
        return Total_;
    }

private:
    ui64 Current_ = 0;
    ui64 Total_ = 0;
};

} // namespace NDoom
