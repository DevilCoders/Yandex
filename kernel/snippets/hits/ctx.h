#pragma once

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

class IInputStream;
class IOutputStream;

namespace NSnippets
{
    namespace NProto
    {
        class THitsCtx;
    }

    struct THitsKind
    {
        enum Type
        {
            Nothing = 0,
            TextHit = 1,
            LinkHit = 2,
        };
    };

    struct THitsInfo : TAtomicRefCount<THitsInfo>
    {
        THitsKind::Type Kind = THitsKind::Nothing;
        TVector<ui16> TextSentsPlain; //old contexts
        TVector<ui16> MoreTextSentsPlain; //even older contexts
        TVector<ui16> THSents;
        TVector<ui16> THMasks;
        TVector<float> FormWeights;
        TVector<ui16> LinkHits;
        bool IsNav = false;
        bool IsRoot = false;
        i64 DocStaticSig = 0;
        bool WatchVideo = false;
        bool IsTurkey = false;
        bool IsSpok = false;
        bool IsFakeForBan = false;
        bool IsFakeForRedirect = false;

        bool IsGoogle() const
        {
            return Kind == THitsKind::LinkHit;
        }
        void Load(IInputStream& str);
        void Save(IOutputStream& str) const;
        void Save(NProto::THitsCtx& res) const;
        void Load(const NProto::THitsCtx& res);
        TString DebugString() const;
    };

    typedef TIntrusivePtr<THitsInfo> THitsInfoPtr;

    inline bool IsByLinkHitsPtr(const THitsInfoPtr& hits) {
        return hits.Get() && hits->IsGoogle();
    }
}

