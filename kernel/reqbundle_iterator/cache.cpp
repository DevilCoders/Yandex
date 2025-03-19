#include "cache.h"
#include "cache_traits.h"

using namespace NReqBundleIterator;

namespace {
    void InitDefaultHashers(NReqBundleIterator::TRBIteratorsHashers& hashers) {
        hashers.TrHasher.Reset(new TRBHasher(DefaultCacheOptions()));
        hashers.LrHasher.Reset(new TRBHasher(DefaultCacheOptions()));
        hashers.AnnHasher.Reset(new TRBHasher(DefaultCacheOptions()));
        hashers.FactorAnnHasher.Reset(new TRBHasher(DefaultCacheOptions()));
        hashers.LinkAnnHasher.Reset(new TRBHasher(DefaultCacheOptions()));
    }

    void InitShrinkHashers(NReqBundleIterator::TRBIteratorsHashers& hashers, size_t n) {
        hashers.TrHasher.Reset(new TRBHasher(ShrinkCacheOptions(n)));
        hashers.LrHasher.Reset(new TRBHasher(ShrinkCacheOptions(n)));
        hashers.AnnHasher.Reset(new TRBHasher(ShrinkCacheOptions(n)));
        hashers.FactorAnnHasher.Reset(new TRBHasher(ShrinkCacheOptions(n)));
        hashers.LinkAnnHasher.Reset(new TRBHasher(ShrinkCacheOptions(n)));
    }

    void InitGrowHashers(NReqBundleIterator::TRBIteratorsHashers& hashers, size_t n) {
        hashers.TrHasher.Reset(new TRBHasher(GrowCacheOptions(n)));
        hashers.LrHasher.Reset(new TRBHasher(GrowCacheOptions(n)));
        hashers.AnnHasher.Reset(new TRBHasher(GrowCacheOptions(n)));
        hashers.FactorAnnHasher.Reset(new TRBHasher(GrowCacheOptions(n)));
        hashers.LinkAnnHasher.Reset(new TRBHasher(GrowCacheOptions(n)));
    }
} // namespace

namespace NReqBundleIterator {
    void TRBIteratorsHashers::Init(EHashersSize hSize) {
        switch (hSize) {
            case HsNull: {
                break;
            }
            case HsShrink1: {
                InitShrinkHashers(*this, 2);
                break;
            }
            case HsShrink2: {
                InitShrinkHashers(*this, 4);
                break;
            }
            case HsShrink3: {
                InitShrinkHashers(*this, 8);
                break;
            }
            case HsGrow1: {
                InitGrowHashers(*this, 2);
                break;
            }
            case HsGrow2: {
                InitGrowHashers(*this, 4);
                break;
            }
            case HsGrow3: {
                InitGrowHashers(*this, 8);
                break;
            }
            default: {
                Y_VERIFY_DEBUG(HsDefault == hSize, "Unknown hashers size: %d", (int)hSize);
                InitDefaultHashers(*this);
                break;
            }
        }
    }
} // NReqBundleIterator
