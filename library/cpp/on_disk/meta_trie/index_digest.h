#pragma once

#include "metatrie_base.h"

#include <library/cpp/containers/comptrie/comptrie.h>
#include <util/generic/hash.h>
#include <util/digest/city.h>
#include <util/string/util.h>
#include <util/ysaveload.h>

namespace NMetatrie {
    const ui64 CURRENT_VERSION = 2;

    ui64 GetPrimeBase(ui64 sz, ui64 portionsz);
    ui64 GetPrimeBase(ui64 n);

    inline ui32 MakeIndex(TStringBuf key, ui32 base, size_t prefixLength) {
        return CityHash64(key.data(), Min<size_t>(key.size(), prefixLength)) % base;
    }

    struct TDigestIndex : IIndex {
        typedef TVector<ui64> TIndex;

        ui64 Version;
        ui64 Base;
        ui64 HashedPrefixLength;

        TVector<ui64> Index;

        TDigestIndex(TBlob b)
            : Version()
            , Base()
            , HashedPrefixLength(TStringBuf::npos)
        {
            TMemoryInput min(b.Data(), b.Size());
            ::Load(&min, Version);
            if (Version > CURRENT_VERSION)
                ythrow TMetatrieException() << "unsupported index version: " << Version;
            ::Load(&min, Base);
            ::Load(&min, Index);
            if (Version >= 2)
                ::Load(&min, HashedPrefixLength);
            if (Index.size() != Base)
                ythrow TMetatrieException() << "index size " << Index.size() << " != base " << Base;
        }

        EIndexType GetType() const override {
            return IT_DIGEST;
        }

        TString Report() const override {
            return Sprintf("ver:%lu, base:%lu, idx:%lu, hashedPrefixLen:%lu", Version, Base, Index.size(), HashedPrefixLength);
        }

        ui64 GetIndex(TStringBuf key) const override {
            return Index[MakeIndex(key, Base, HashedPrefixLength)];
        }
    };

    struct TDigestIndexBuilder : IIndexBuilder {
        ui64 Base;
        ui64 HashedPrefixLength;
        TDigestIndex::TIndex Index;

        TDigestIndexBuilder()
            : Base(1)
            , HashedPrefixLength(TStringBuf::npos)
        {
        }

        explicit TDigestIndexBuilder(ui32 base, ui64 hashedPrefixLength = TStringBuf::npos)
            : Base(Min<ui64>(Max<ui32>(base, 1), Max<ui32>()))
            , HashedPrefixLength(hashedPrefixLength)
        {
            Index.resize(Base, Max<ui64>());
        }

        EIndexType GetType() const override {
            return IT_DIGEST;
        }

        TString GenerateMetaKey(TStringBuf key) const override {
            return Sprintf("%08x", MakeIndex(key, Base, HashedPrefixLength));
        }

        void Add(TStringBuf f, TStringBuf l, size_t t) override {
            ui64 idx = MakeIndex(f, Base, HashedPrefixLength);
            if (idx != MakeIndex(l, Base, HashedPrefixLength))
                ythrow TMetatrieException() << "Indices of both first and last subtrie keys should be equal";
            Index[idx] = t;
        }

        TBlob Commit() override {
            TBuffer b;
            {
                TBufferOutput bout(b);
                ::Save(&bout, CURRENT_VERSION);
                ::Save(&bout, Base);
                ::Save(&bout, Index);
                ::Save(&bout, HashedPrefixLength);
            }
            return TBlob::FromBuffer(b);
        }
    };

}
