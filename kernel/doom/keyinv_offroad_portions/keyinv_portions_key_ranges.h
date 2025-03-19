#pragma once

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>
#include <util/ysaveload.h>

#include <array>

namespace NDoom {


class TKeyInvPortionsKeyRanges {
public:
    TKeyInvPortionsKeyRanges() = default;

    TKeyInvPortionsKeyRanges(TVector<TString> startKeys)
        : StartKeys_(std::move(startKeys))
    {
        Init();
    }

    ui32 Range(TStringBuf s) const {
        Y_ENSURE(!s.empty());
        const ui16 firstChar = static_cast<ui8>(s[0]);
        size_t index = UpperBound(StartKeys_.begin() + FirstIndexByChar_[firstChar], StartKeys_.begin() + FirstIndexByChar_[firstChar + 1], s) - StartKeys_.begin();
        Y_VERIFY(index > 0);
        return index - 1;
    }

    ui32 NumRanges() const {
        return StartKeys_.size();
    }

    const TVector<TString>& StartKeys() const {
        return StartKeys_;
    }

    void Save(IOutputStream* out) const {
        ::Save(out, StartKeys_);
    }

    void Load(IInputStream* in) {
        ::Load(in, StartKeys_);
        Init();
    }

    static const TKeyInvPortionsKeyRanges EmptyKeyRanges;

private:
    void Init() {
        Y_ENSURE(!StartKeys_.empty());
        Y_ENSURE(StartKeys_[0].empty());
        if (StartKeys_.size() == 1) {
            FirstIndexByChar_.fill(1);
            return;
        }
        ui16 lastChar = 0;
        for (size_t i = 1; i < StartKeys_.size(); ++i) {
            Y_ENSURE(StartKeys_[i - 1] < StartKeys_[i]);
            ui16 curChar = static_cast<ui8>(StartKeys_[i][0]);
            while (lastChar <= curChar) {
                FirstIndexByChar_[lastChar++] = i;
            }
        }
        while (lastChar <= 256) {
            FirstIndexByChar_[lastChar++] = StartKeys_.size();
        }
    }

    TVector<TString> StartKeys_;
    std::array<size_t, 257> FirstIndexByChar_ = {{}};
};

} // namespace NDoom
