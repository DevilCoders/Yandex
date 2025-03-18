#pragma once

#include <library/cpp/reverse_geocoder/core/edge.h>
#include <library/cpp/reverse_geocoder/core/part.h>

#include <util/generic/noncopyable.h>
#include <util/generic/vector.h>
#include <util/generic/algorithm.h>

#include <library/cpp/testing/unittest/registar.h>

namespace NReverseGeocoder {
    class TEdgeMaker: public TNonCopyable {
    public:
        TEdge MakeEdge(const TPoint& a, const TPoint& b) {
            Points_.push_back(a);
            Points_.push_back(b);
            return TEdge(Points_.size() - 2, Points_.size() - 1);
        }

        const TPoint* Points() const {
            return Points_.data();
        }

    protected:
        TVector<TPoint> Points_;
    };

    struct TPartHelper: public TPart {
        TNumber EdgeRefsNumber;

        template <typename TData>
        bool Contains(const TPoint& point, const TData& data) const {
            return TPart::Contains(point, EdgeRefsNumber, data.EdgeRefs(), data.Edges(),
                                   data.Points());
        }
    };

    class TPartMaker: public TEdgeMaker {
    public:
        TPartHelper MakePart(TCoordinate coordinate, const TVector<TEdge>& edges) {
            TPartHelper helper;
            helper.Coordinate = coordinate;
            helper.EdgeRefsOffset = EdgeRefs_.size();

            for (const TEdge& e : edges) {
                EdgeRefs_.push_back(Edges_.size());
                Edges_.push_back(e);
            }

            helper.EdgeRefsNumber = EdgeRefs_.size();

            auto cmp = [&](const TRef& a, const TRef& b) {
                const TEdge& e1 = Edges_[a];
                const TEdge& e2 = Edges_[b];
                return e1.Lower(e2, Points_.data());
            };

            Sort(EdgeRefs_.begin() + helper.EdgeRefsOffset, EdgeRefs_.end(), cmp);

            return helper;
        }

        const TEdge* Edges() const {
            return Edges_.data();
        }

        const TRef* EdgeRefs() const {
            return EdgeRefs_.data();
        }

    protected:
        TVector<TEdge> Edges_;
        TVector<TRef> EdgeRefs_;
    };

}
