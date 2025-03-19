#pragma once
#include <kernel/text_machine/interface/feature.h>

#include <util/generic/hash_set.h>
#include <util/generic/array_ref.h>

namespace NTextMachine {
namespace NCore {
    class TFeaturesFilter{
    public:
        enum class EMode{
            AcceptedNone,
            AcceptedAll,
            FilterBySet
        };

        using TDataPtr = const THashSet<TFFId>*;
    private:
        EMode Mode = EMode::AcceptedNone;
        TDataPtr Set = nullptr;
    public:
        TFeaturesFilter(EMode mode, TDataPtr set = nullptr)
            : Mode(mode)
            , Set(set)
        {
            Y_ASSERT(mode != EMode::FilterBySet || set != nullptr);
        }

        bool Empty() const {
            return Mode == EMode::AcceptedNone;
        }

        bool IsAccepted(const TFFId& id) const {
            switch (Mode) {
                case EMode::AcceptedNone: {
                    return false;
                }
                case EMode::AcceptedAll: {
                    return true;
                }
                case EMode::FilterBySet: {
                    return Set->contains(id);
                }
                default: {
                    return false;
                }
            }
        }

        void Apply(TArrayRef<TOptFeature>& features) const {
           ApplyImpl(features, [](const TOptFeature& f){return f.GetId();});
        }

        void Apply(TOptFeatures& features) const {
           auto region = MakeArrayRef(features);
           Apply(region);
           features.resize(region.size());
        }

        void Apply(TArrayRef<TFFId>& ids) const {
            ApplyImpl(ids, [](const TFFId& id){return id;});
        }

        void Apply(TFFIds& ids) const {
            auto region = MakeArrayRef(ids);
            Apply(region);
            ids.resize(region.size());
        }

    private:
        template <typename T, typename A>
        void ApplyImpl(TArrayRef<T>& region, const A& acc = A()) const {
            switch (Mode) {
                case EMode::AcceptedNone: {
                    region = TArrayRef<T>();
                    break;
                }
                case EMode::AcceptedAll: {
                    break;
                }
                case EMode::FilterBySet: {
                    region = TArrayRef<T>(region.begin(), std::remove_if(region.begin(), region.end(),
                        [this, acc] (const T& el) {
                            return !IsAccepted(acc(el));
                        }));
                    break;
                }
            }
        }
    };

    inline TFeaturesFilter MakeAcceptAllFeaturesFilter() {
        return TFeaturesFilter(TFeaturesFilter::EMode::AcceptedAll);
    }

    inline TFeaturesFilter MakeAcceptNoneFeaturesFilter() {
        return TFeaturesFilter(TFeaturesFilter::EMode::AcceptedNone);
    }

    inline TFeaturesFilter MakeFeaturesFilterFromSetRef(const THashSet<TFFId>& set) {
        return TFeaturesFilter(TFeaturesFilter::EMode::FilterBySet, &set);
    }
} // NCore
} // NTextMachine
