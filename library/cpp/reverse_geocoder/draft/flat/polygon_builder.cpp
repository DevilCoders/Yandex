#include "polygon_builder.h"

#include <library/cpp/reverse_geocoder/draft/core/point.h>

#include <util/generic/algorithm.h>
#include <util/generic/vector.h>

namespace NReverseGeocoder::NDraft::NFlat {
    namespace {
        struct TEdge {
            TPoint Beg = {};
            TPoint End = {};
        };

        struct TCheckPoint {
            i32 X = 0;
            size_t EdgeIndex = 0;
            bool IsStart = false;
        };

        TVector<TEdge> MakeEdges(const NProto::TPolygon& polygon) {
            TVector<TEdge> result;
            result.reserve(polygon.GetLocations().size());

            for (int i = 0; i < polygon.GetLocations().size(); ++i) {
                TEdge edge{
                    LocationToPoint(CreateLocation(polygon.GetLocations(i))),
                    LocationToPoint(CreateLocation(polygon.GetLocations((i + 1) % polygon.GetLocations().size())))};

                if (edge.Beg.X == edge.End.X) {
                    continue;
                }

                if (edge.Beg.X > edge.End.X) {
                    DoSwap(edge.Beg, edge.End);
                }

                result.push_back(edge);
            }

            return result;
        }

        TVector<TCheckPoint> MakeCheckPoints(const TVector<TEdge> edges) {
            TVector<TCheckPoint> result;
            result.reserve(edges.size() * 2);

            for (size_t i = 0; i < edges.size(); ++i) {
                result.push_back({edges[i].Beg.X, i, true});
                result.push_back({edges[i].End.X, i, false});
            }

            auto cmp = [](const auto& a, const auto& b) {
                return a.X < b.X;
            };

            Sort(result.begin(), result.end(), cmp);

            return result;
        }

    }

    void TPolygonBuilder::Build(const NProto::TPolygon& polygon, IOutputStream& outputStream) {
        Y_UNUSED(outputStream);

        const TVector<TEdge> edges = MakeEdges(polygon);
        const TVector<TCheckPoint> checkPoints = MakeCheckPoints(edges);

        TVector<size_t> erase;
        TVector<size_t> currentEdges;

        for (size_t start = 0, finish = 0; start < checkPoints.size(); start = finish) {
            while (finish < checkPoints.size() && checkPoints[finish].X == checkPoints[start].X) {
                ++finish;
            }

            for (size_t i = start; i < finish; ++i) {
                if (checkPoints[i].IsStart) {
                    currentEdges.push_back(checkPoints[i].EdgeIndex);
                }
            }

            if (finish != checkPoints.size()) {

            }

            erase.clear();
            for (size_t i = start; i < finish; ++i) {
                if (!checkPoints[i].IsStart) {
                    erase.push_back(checkPoints[i].EdgeIndex);
                }
            }
            Sort(erase.begin(), erase.end());

            for (size_t i = 0; i < currentEdges.size(); ) {
                if (BinarySearch(erase.begin(), erase.end(), currentEdges[i])) {
                    DoSwap(currentEdges[i], currentEdges.back());
                    currentEdges.pop_back();
                } else {
                    ++i;
                }
            }
        }
    }

}
