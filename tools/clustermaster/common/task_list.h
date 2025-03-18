#pragma once

#include <util/generic/string.h>

template <class T = TVector<int>>
TString JoinTaskList(const TString& sep, const T& tasks, size_t count = 5) {
    TString result;
    size_t i = 0;
    if (count == 0 || tasks.size() <= count) {
        for (const auto &id : tasks) {
            if (i) {
                result += sep;
            }
            result += ToString(id);
            ++i;
        }
    }
    else {
        for (const auto &id : tasks) {
            if (i >= count) {
                break;
            }
            result += ToString(id) + sep;
            ++i;
        }
        result += "...";
    }
    return result;
}
