#pragma once

#include "gen_geo_data.h"

#include <util/generic/hash_set.h>
#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>

namespace NReverseGeocoder {
    namespace NGenerator {
        class TMutGeoData: public TGenGeoData, public TNonCopyable {
#define GEO_BASE_DEF_VAR(TVar, Var)           \
public:                                       \
    const TVar& Var() const override {        \
        return Var##_;                        \
    }                                         \
    void Set##Var(const TVar& Var) override { \
        Var##_ = Var;                         \
    }                                         \
                                              \
private:                                      \
    TVar Var##_;

#define GEO_BASE_DEF_ARR(TArr, Arr)                               \
public:                                                           \
    const TArr* Arr() const override {                            \
        return Arr##_.data();                                     \
    }                                                             \
    TArr* Mut##Arr() override {                                   \
        return Arr##_.data();                                     \
    }                                                             \
    TNumber Arr##Number() const override {                        \
        return Arr##_.size();                                     \
    }                                                             \
    void Arr##Append(const TArr& a) override {                    \
        if (Arr##_.size() == std::numeric_limits<TNumber>::max()) \
            std::abort();                                         \
        Arr##_.push_back(a);                                      \
    }                                                             \
                                                                  \
private:                                                          \
    TVector<TArr> Arr##_;

            GEO_BASE_DEF_GEO_DATA

#undef GEO_BASE_DEF_VAR
#undef GEO_BASE_DEF_ARR

        public:
            TMutGeoData();

            TRef Insert(const TPoint& p) override {
                if (PointRef_.find(p) == PointRef_.end()) {
                    PointRef_[p] = Points_.size();
                    Points_.push_back(p);
                }
                return PointRef_[p];
            }

            TRef Insert(const TEdge& e) override {
                if (EdgeRef_.find(e) == EdgeRef_.end()) {
                    EdgeRef_[e] = Edges_.size();
                    Edges_.push_back(e);
                }
                return EdgeRef_[e];
            }

            TRef Insert(const TString& s) override {
                if (BlobRef_.find(s) == BlobRef_.end()) {
                    BlobRef_[s] = Blobs_.size();
                    for (size_t i = 0; i < s.size(); ++i)
                        Blobs_.push_back(s.at(i));
                    Blobs_.push_back('\0');
                }
                return BlobRef_[s];
            }

        private:
            template <typename TType>
            struct THash64 {
                ui64 operator()(const TType& x) const {
                    static_assert(sizeof(x) == sizeof(ui64), "Data sizes must be equal to 8 bytes");
                    return NumericHash(*((const ui64*)&x));
                }
            };

            THashMap<TEdge, TRef, THash64<TEdge>> EdgeRef_;
            THashMap<TPoint, TRef, THash64<TPoint>> PointRef_;
            THashMap<TString, TRef> BlobRef_;
        };

    }
}
