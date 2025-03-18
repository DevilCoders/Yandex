#pragma once

#include <util/generic/array_ref.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/generic/yexception.h>

namespace NVowpalWabbit {
    ui32 UniformHash(const void* key, size_t len, ui32 seed) noexcept;
    ui32 HashString(const char* begin, const char* end, ui32 seed) noexcept;
    inline ui32 HashString(const TStringBuf s, ui32 seed) noexcept {
        return HashString(s.data(), s.data() + s.size(), seed);
    }

    struct ICallbacks {
        virtual ~ICallbacks() = default;

        // called on each ngram == tokens[idxSt], ..., tokens[idxEnd - 1] and hashVal == vw_hash(ngram)
        virtual void OnCalcedHash(size_t idxSt, size_t idxEnd, ui32 hashVal) = 0;

        virtual void OnCalcHashesStart() = 0;
    };

    constexpr ui32 VW_CONST_HASH = 11650396;
    constexpr ui32 VW_QUADRATIC_CONST = 27942141;

    class THashCalcer {
    public:
        template <typename TStringType>
        static void CalcHashes(const TStringBuf ns, const TVector<TStringType>& words, ui32 ngrams, TVector<ui32>& hashes) {
            CalcHashesImpl<TStringType, false, false>(ns, words, ngrams, hashes, nullptr);
        }

        template <typename TStringType>
        static void CalcHashesWithAppend(const TStringBuf ns, const TVector<TStringType> &words, ui32 ngrams,
                                         TVector<ui32> &hashes) {
            CalcHashesImpl<TStringType, false, true>(ns, words, ngrams, hashes, nullptr);
        }

        template <typename TStringType>
        static void CalcHashesWithCallbacks(const TStringBuf ns, const TVector<TStringType>& words, ui32 ngrams,
                                            TVector<ui32>& hashes, ICallbacks* callbacks = nullptr) {
            callbacks
                ? CalcHashesImpl<TStringType, true, false>(ns, words, ngrams, hashes, callbacks)
                : CalcHashesImpl<TStringType, false>(ns, words, ngrams, hashes, nullptr);
        }

    protected:
        template <typename TStringType, bool EnabledCallbacks, bool AppendHashes>
        static void CalcHashesImpl(TStringBuf ns, const TVector<TStringType>& words, ui32 ngrams,
                                   TVector<ui32>& hashes, ICallbacks* callbacks)
        {
            Y_ASSERT(ngrams >= 1);
            Y_ASSERT(EnabledCallbacks == (callbacks != nullptr));

            ngrams = Min<ui64>(words.size(), ngrams);
            const size_t hashesCount = ngrams * words.size() - (ngrams * (ngrams - 1)) / 2;
            if (AppendHashes) {
                hashes.reserve(hashes.size() + hashesCount);
            } else {
                hashes.clear();
                hashes.reserve(hashesCount);
            }
            const ui32 nsHash = HashString(ns, 0);
            const ui32* const hashesStart = AppendHashes ? hashes.data() + hashes.size() : hashes.data();

            if (EnabledCallbacks) {
                callbacks->OnCalcHashesStart();
            }

            for (size_t i = 0, iEnd = words.size(); i < iEnd; ++i) {
                const auto& word = words[i];
                hashes.push_back(HashString(word.data(), word.data() + word.size(), nsHash));
                if (EnabledCallbacks) {
                    callbacks->OnCalcedHash(i, i + 1, hashes.back());
                }
            }

            if (ngrams < 2) {
                return;
            }

            for (size_t i = 0, iEnd = words.size(); i < iEnd; ++i) {
                ui32 ngramHash = hashesStart[i];
                for (size_t j = i + 1, jEnd = Min(iEnd, i + ngrams); j < jEnd; ++j) {
                    ngramHash = ngramHash * VW_QUADRATIC_CONST + hashesStart[j];
                    hashes.push_back(ngramHash);
                    if (EnabledCallbacks) {
                        callbacks->OnCalcedHash(i, j + 1, hashes.back());
                    }
                }
            }
        }
    };

    template <class TVwModel>
    class TPredictor : public THashCalcer {
    private:
        const TVwModel& Model;

    public:
        explicit TPredictor(const TVwModel& model)
        : Model(model)
        {
            Y_ENSURE(model.GetBits() > 10);
        }

        TPredictor(TVwModel&& model) = delete;

        float GetConstPrediction() const noexcept {
            return Model[VW_CONST_HASH];
        }

        template <typename TStringType>
        float Predict(const TStringBuf ns, const TVector<TStringType>& words, ui32 ngrams = 1) const {
            return PredictImpl<TStringType, false>(ns, words, ngrams, nullptr);
        }

        template <typename TStringType>
        float PredictWithCallbacks(const TStringBuf ns, const TVector<TStringType>& words,
                                   ui32 ngrams = 1, ICallbacks* callbacks = nullptr) const {
            return callbacks ? PredictImpl<TStringType, true>(ns, words, ngrams, callbacks) : PredictImpl<TStringType, false>(ns, words, ngrams, nullptr);
        }

        float Predict(const TVector<ui32>& hashes) const noexcept {
            return Model[hashes];
        }

    private:
        template <typename TStringType, bool EnabledCallbacks>
        float PredictImpl(TStringBuf ns, const TVector<TStringType>& words, ui32 ngrams, ICallbacks* callbacks) const {
            TVector<ui32> hashes;
            CalcHashesImpl<TStringType, EnabledCallbacks, false>(ns, words, ngrams, hashes, callbacks);
            return Predict(hashes);
        }
    };

    class TModel;
}

using TVowpalWabbitPredictor = NVowpalWabbit::TPredictor<NVowpalWabbit::TModel>;
