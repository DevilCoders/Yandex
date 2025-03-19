#pragma once
/*! @file dochandlehash.h
 * Here are defined the comparators and hashers for TDocHandle.
 */

#include <kernel/urlid/doc_handle.h>
#include <util/digest/numeric.h>
#include <util/stream/output.h>

//==================== Hashers ====================
//! use DocHash only
struct TDocHandleHashIgnoreRoute {
    inline size_t operator() (const TDocHandle& handle) const noexcept {
        const auto& hash = handle.DocHash;
        return std::holds_alternative<TString>(hash)? ::THash<TString>()(std::get<TString>(hash)) : std::get<TDocHandle::THash>(hash);
    }
};

//! use DocHash + DocRoute
struct TDocHandleHash {
    inline size_t operator() (const TDocHandle& handle) const noexcept {
        return CombineHashes(static_cast<ui64>(handle.DocRoute.Raw()),
                             TDocHandleHashIgnoreRoute()(handle));
    }
};

//! use DocRoute iff !handle.IsUnique()
struct TDocHandleZHash {
    inline size_t operator() (const TDocHandle& handle) const noexcept {
        return handle.IsUnique() ? TDocHandleHashIgnoreRoute()(handle) : TDocHandleHash()(handle);
    }
};

//! use DocHash + DocRoute + IndexGeneration
struct TDocHandleHashWithIdx {
    inline size_t operator() (const TDocHandle& handle) const noexcept {
        return CombineHashes(TDocHandleHash()(handle),
                             static_cast<ui64>(handle.IndexGeneration));
    }
};

//==================== Comparators ====================
//! use DocHash only
struct TDocHandleEqualIgnoreRoute {
    inline bool operator() (const TDocHandle& a, const TDocHandle& b) const noexcept {
        return a.DocHash == b.DocHash;
    }
};

//! use DocRoute iff both docids are not unique
struct TDocHandleEqualZ {
    inline bool operator() (const TDocHandle& a, const TDocHandle& b) const noexcept {
        return a.DocHash == b.DocHash
            && a.IsUnique() == b.IsUnique()
            && (a.IsUnique() || (a.DocRoute == b.DocRoute));
    }
};

struct TDocHandleLessIgnoreRoute {
    inline bool operator() (const TDocHandle& a, const TDocHandle& b) const noexcept {
        using THash = TDocHandle::THash;
        bool aBinary = std::holds_alternative<THash>(a.DocHash);
        bool bBinary = std::holds_alternative<THash>(b.DocHash);
        if (aBinary != bBinary)
            return aBinary < bBinary;
        if (aBinary)
            return std::get<THash>(a.DocHash) < std::get<THash>(b.DocHash);
        else
            return std::get<TString>(a.DocHash) < std::get<TString>(b.DocHash);
    }
};

//! Arbitrary strict order on TDocHandles. IndexGeneration is ignored here.
struct TDocHandleLess {
    inline bool operator() (const TDocHandle& a, const TDocHandle& b) const noexcept {
        if (a.DocRoute.Raw() != b.DocRoute.Raw())
            return a.DocRoute.Raw() < b.DocRoute.Raw();
        else
            return TDocHandleLessIgnoreRoute()(a, b);
    }
};

