#pragma once

#include "abstract.h"
#include "traits.h"

namespace NGraph {
    struct TNothing {
    };

    template <class VertexProps = TNothing,
              class EdgeProps = TNothing>
    class TGraph: public TAbstractGraph<TDefaultTraits<VertexProps, EdgeProps>> {
    public:
        typedef TAbstractGraph<TDefaultTraits<VertexProps, EdgeProps>> TBase;
        typedef typename TBase::TCount TCount;
        typedef typename TBase::TVertexProps TVertexProps;
        typedef typename TBase::TStringType TStringType;
        template <EDirection Direction>
        using TConstAdjacentIterator = typename TBase::template TConstAdjacentIterator<Direction>;

    public:
        TGraph(const TBase& base)
            : TBase(base)
        {
        }

        TGraph(TCount numVertices = 0)
            : TBase(numVertices)
        {
        }

        TGraph(TCount numVertices, TVertexProps const& props)
            : TBase(numVertices, props)
        {
        }
    };

}

template <>
inline void In<NGraph::TNothing>(IInputStream&, NGraph::TNothing&) {
}
template <>
inline void Out<NGraph::TNothing>(IOutputStream&, NGraph::TNothing const&) {
}
