#pragma once

#include <util/generic/yexception.h>

#include <iterator>
#include <type_traits>


namespace NAntiRobot {


template <typename TIter, typename F>
void MapInto(TIter begin, TIter end, const F& f) {
    Y_UNUSED(f);
    Y_ENSURE(begin == end, "Expected an empty range");
}

template <typename TRange, typename F>
void MapInto(const TRange& range, const F& f) {
    MapInto(range.begin(), range.end(), f);
}


template <typename TIter, typename F, typename TOutput, typename... TOutputs>
void MapInto(TIter begin, TIter end, const F& f, TOutput* output, TOutputs*... outputs) {
    Y_ENSURE(begin != end, "Expected a non-empty range");
    f(*begin, output);
    MapInto(std::next(begin), end, f, outputs...);
}

template <typename TRange, typename F, typename TOutput, typename... TOutputs>
void MapInto(const TRange& range, const F& f, TOutput* output, TOutputs*... outputs) {
    MapInto(range.begin(), range.end(), f, output, outputs...);
}


template <typename TRange, typename... TOutputs>
void ParseSplitTokensInto(const TRange& range, TOutputs*... outputs) {
    MapInto(
        range,
        [] (const auto& state, auto* output) {
            using TOutput = std::remove_cvref_t<decltype(*output)>;

            if constexpr (std::is_same_v<TOutput, TString>) {
                *output = TString(state.Token());
            } else if constexpr (std::is_same_v<TOutput, TStringBuf>) {
                *output = state.Token();
            } else {
                *output = FromString<TOutput>(state.Token());
            }
        },
        outputs...
    );
}


} // namespace NAntiRobot
