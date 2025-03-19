#pragma once

#include <library/cpp/offroad/key/key_sampler.h>

namespace NDoom {

template <class KeyData, class Vectorizer, class Subtractor>
using TOffroadKeyWadSampler = NOffroad::TKeySampler<KeyData, Vectorizer, Subtractor>;

} // namespace NDoom
