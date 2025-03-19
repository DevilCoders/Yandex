#pragma once
#include "coord.h"
#include "rect.h"
#include <util/generic/ymath.h>
#include <util/generic/maybe.h>
#include <library/cpp/logger/global/global.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/generic/map.h>
#include <util/string/vector.h>
#include <kernel/common_server/util/accessor.h>

class TInternalPointContext {
public:
    enum EInternalPolicySet {
        Simple,
        Guarantee,
        Potentially
    };

    enum EInternalPointType {
        ExternalGuarantee = 1 << 0,
        ExternalPotentially = 1 << 1,
        InternalPotentially = 1 << 2,
        InternalGuarantee = 1 << 3,
    };

private:
    RTLINE_ACCEPTOR(TInternalPointContext, Policy, EInternalPolicySet, EInternalPolicySet::Simple);
    RTLINE_ACCEPTOR(TInternalPointContext, Precision, double, 0);
public:
    template <class TPolyLine>
    bool IsPointInternal(const TPolyLine& line, const typename TPolyLine::TCoord& c) const {
        if (line.IsPointInternal(c)) {
            if (Policy == EInternalPolicySet::Guarantee && Precision > 0) {
                typename TPolyLine::TPosition pos;
                return !line.MatchSimple(c, Precision, pos);
            }
            return true;
        } else {
            if (Policy == EInternalPolicySet::Potentially && Precision > 0) {
                typename TPolyLine::TPosition pos;
                return line.MatchSimple(c, Precision, pos);
            }
            return false;
        }
    }

    template <class TPolyLine>
    EInternalPointType CheckPointInternal(const TPolyLine& line, const typename TPolyLine::TCoord& c) const {
        if (line.IsPointInternal(c)) {
            if (Precision > 0) {
                typename TPolyLine::TPosition pos;
                if (line.MatchSimple(c, Precision, pos)) {
                    return EInternalPointType::InternalPotentially;
                }
            }
            return EInternalPointType::InternalGuarantee;
        } else {
            if (Precision > 0) {
                typename TPolyLine::TPosition pos;
                if (line.MatchSimple(c, Precision, pos)) {
                    return EInternalPointType::ExternalPotentially;
                }
            }
            return EInternalPointType::ExternalGuarantee;
        }
    }
};

template <class C, class TIdxType = ui16>
class TPolyLine {
public:
    using TCoordType = typename C::TCoordType;
    using TCoord = C;
    using TRect = ::TRect<C>;
    static TIdxType GetIdxMax() {
        return Max<TIdxType>();
    }
    class TPosition {
    private:
        double Distance = 0;
        TIdxType Index = Max<TIdxType>();

    public:
        TPosition() = default;
        TPosition(double distance, TIdxType index)
            : Distance(distance)
            , Index(index)
        {
        }

        TPosition Reverse(const TPolyLine<C>& line) const {
            TPosition result = *this;
            if (result.Index == line.Size() - 1) {
                result.Index = 0;
                result.Distance = 0;
                return result;
            }
            CHECK_WITH_LOG(result.Index < line.Coords.size());
            CHECK_WITH_LOG(line.GetLength() >= result.Distance);
            result.Distance = line.GetLength() - result.Distance;
            result.Index = line.Coords.size() - (result.Index + 1) - 1;
            return result;
        }

        Y_FORCE_INLINE TPosition MoveForward(double d) const {
            TPosition result = *this;
            result.Distance += d;
            return result;
        }

        Y_FORCE_INLINE bool operator<(const TPosition& item) const {
            return Distance < item.Distance;
        }

        Y_FORCE_INLINE bool operator<=(const TPosition& item) const {
            return Distance <= item.Distance;
        }

        Y_FORCE_INLINE bool operator>(const TPosition& item) const {
            return Distance > item.Distance;
        }

        Y_FORCE_INLINE bool Defined() const {
            return Index != Max<TIdxType>();
        }

        Y_FORCE_INLINE double GetDistance() const {
            return Distance;
        }

        Y_FORCE_INLINE TIdxType GetIndex() const {
            return Index;
        }

        TString ToString() const {
            return  ::ToString(Distance) + "/" + ::ToString(Index);
        }

        bool Compare(const TPosition& item, double precision) const {
            return (Index == item.Index) && (Abs(Distance - item.Distance) < precision);
        }

        NJson::TJsonValue SerializeToJson() const {
            NJson::TJsonValue result;
            result["dist"] = Distance;
            result["index"] = Index;
            return result;
        }
    };

    class TIterator {
    private:
        TPosition Position;
        const TPolyLine& Line;
        i32 Delta;
        i32 Index;
        ui32 Start;

    public:
        TIterator(const TPolyLine& line, i32 delta, ui32 start)
            : Line(line)
            , Delta(delta)
            , Index(start)
            , Start(start)
        {
            Position = Line.GetPosition(Index);
            if (Delta < 0)
                Position = Position.Reverse(Line);
        }

        void operator++() {
            Index += Delta;
            if (IsValid()) {
                Position = Line.GetPosition(Index);
            }
            if (Delta < 0)
                Position = Position.Reverse(Line);
        }

        bool IsValid() const {
            return (Index >= 0) && (Index < (i32)Line.Size());
        }

        const TPosition& GetPosition() const {
            return Position;
        }

        TCoord GetCoord() const {
            return Line.GetCoord((Delta < 0) ? Position.Reverse(Line) : Position);
        }
    };

    template <class T>
    class TSegmentation {
    public:
        using TObject = T;
        enum EFollowResult { frStop, frEnd, frOK };

    private:
        TMap<TPosition, TObject> Segments;

    public:
        const TMap<TPosition, TObject>& GetSegments() const {
            return Segments;
        }

        Y_FORCE_INLINE bool GetObject(const TPosition& position, TObject& object) const {
            auto it = Segments.upper_bound(position);
            if (it == Segments.begin()) {
                return false;
            }
            object = (--it)->second;
            return true;
        }

        Y_FORCE_INLINE TPosition End() const {
            auto it = Segments.rbegin();
            if (it == Segments.rend())
                return TPosition();
            return it->first;
        }

        void AddObject(const TPosition& pos, const TObject& obj) {
            CHECK_WITH_LOG(Segments.insert(std::make_pair(pos, obj)).second);
        }

        template <class TGuide>
        EFollowResult FollowPosition(const TPolyLine& line, const TPosition& from, TGuide& guide, double startGapSize, double delta, TPosition& result, TMaybe<TPosition>& lastReal) const {
            lastReal = from;

            if (!Segments.size())
                return frStop;

            double gapSize = startGapSize;

            bool predFollow = false;

            auto it = Segments.upper_bound(from);

            if (it != Segments.begin()) {
                --it;
                predFollow = guide.Follow(it->second);
                CHECK_WITH_LOG(it->first <= from);
                ++it;
            } else {
                predFollow = false;
            }

            result = from;

            bool atFirst = true;

            for (; it != Segments.end(); ++it) {
                if (!predFollow) {
                    if (gapSize + (it->first.GetDistance() - result.GetDistance()) > delta)
                        return atFirst ? frStop : frOK;
                    gapSize += (it->first.GetDistance() - result.GetDistance());
                } else {
                    lastReal = it->first;
                    gapSize = 0;
                }
                result = it->first;
                atFirst = false;
                predFollow = guide.Follow(it->second);
            }
            return (Abs(line.GetLength() - result.GetDistance()) > 1e-2) ? frOK : frEnd;
        }

    };

private:
    TVector<C> Coords;

    struct TLinkedRect {
        TIdxType IdxMinX = Max<TIdxType>();
        TIdxType IdxMinY = Max<TIdxType>();
        TIdxType IdxMaxX = Max<TIdxType>();
        TIdxType IdxMaxY = Max<TIdxType>();
        TRect GetRect(const TVector<TCoord>& coords) const {
            CHECK_WITH_LOG(IdxMinX != Max<TIdxType>());
            TRect result;
            result.Min.X = coords[IdxMinX].X;
            result.Min.Y = coords[IdxMinY].Y;
            result.Max.X = coords[IdxMaxX].X;
            result.Max.Y = coords[IdxMaxY].Y;
            return result;
        }

        void Link(const TVector<TCoord>& coords, const TIdxType idx, TRect& currentRect){
            if (IdxMinY == Max<TIdxType>()) {
                IdxMinY = IdxMinX = IdxMaxY = IdxMaxX = idx;
                currentRect = TRect(coords[idx]);
            } else {
                const TCoord& c = coords[idx];
                if (currentRect.Min.X > c.X) {
                    currentRect.Min.X = c.X;
                    IdxMinX = idx;
                }
                if (currentRect.Min.Y > c.Y) {
                    currentRect.Min.Y = c.Y;
                    IdxMinY = idx;
                }
                if (currentRect.Max.X < c.X) {
                    currentRect.Max.X = c.X;
                    IdxMaxX = idx;
                }
                if (currentRect.Max.Y < c.Y) {
                    currentRect.Max.Y = c.Y;
                    IdxMaxY = idx;
                }
            }
        }
    };

    TLinkedRect LinkedRect;
    double Length = 0;
    TVector<ui32> PositionsFraction;
    using TSelf = TPolyLine<C>;

public:

    void AttachMeTo(TVector<C>& coords, const bool isFwd) const {
        AttachLines(coords, Coords, isFwd);
    }

    static void AttachLines(TVector<C>& coords, const TVector<C>& coordsLine, const bool isFwd) {
        if (coordsLine.empty())
            return;
        coords.reserve(coords.size() + coordsLine.size());

        const auto builder = [&coords](auto b, auto e) {
            if (coords.empty()) {
                coords.insert(coords.end(), b, e);
            } else {
                if (coords.empty() || !coords.back().Compare(*b, 1e-5)) {
                    coords.emplace_back(*b);
                }
                coords.insert(coords.end(), b + 1, e);
            }
        };

        if (isFwd) {
            builder(coordsLine.begin(), coordsLine.end());
        } else {
            builder(coordsLine.rbegin(), coordsLine.rend());
        }
    }

    Y_FORCE_INLINE bool IsExactPosition(const TPosition& pos, const double precision = 1e-5) const {
        return Abs(pos.GetDistance() - GetPositionLength(pos.GetIndex())) < precision;
    }

    Y_FORCE_INLINE double GetPositionLength(const TIdxType index) const {
        return Length * PositionsFraction[index] / Max<ui32>();
    }

    TString SerializeToString() const {
        return TCoord::SerializeVector(Coords);
    }

    static TSelf BuildFromString(const TString& line) {
        TVector<TCoord> coords;
        CHECK_WITH_LOG(TCoord::DeserializeVector(line, coords));
        return TSelf(coords);
    }

    NRTYGeometry::TPolyLine SerializeToProto() const {
        NRTYGeometry::TPolyLine result;
        for (auto&& it : Coords) {
            NRTYGeometry::TSimpleCoord* coord = result.AddCoords();
            *coord = it.Serialize();
        }
        return result;
    }

    TIterator ForwardIterator() const {
        return TIterator(*this, 1, 0);
    }

    TIterator BackwardIterator() const {
        return TIterator(*this, -1, Size() - 1);
    }

    template <class T>
    NJson::TJsonValue SegmentationToJson(const TSegmentation<T>& segmentation, bool fwd) const {
        NJson::TJsonValue result(NJson::JSON_ARRAY);
        TIterator it = fwd ? ForwardIterator() : BackwardIterator();
        bool first = true;
        for (auto&& i : segmentation.GetSegments()) {
            TPosition pos = i.first;
            while (it.IsValid() && pos > it.GetPosition()) {
                if (!first) {
                    NJson::TJsonValue json = pos.SerializeToJson();
                    auto c = it.GetCoord();
                    json["x"] = c.X;
                    json["y"] = c.Y;
                    result.AppendValue(json);
                }
                ++it;
            }
            first = false;
            {
                NJson::TJsonValue jsonPos = pos.SerializeToJson();
                TCoord c = GetCoord(fwd ? pos : pos.Reverse(*this));
                jsonPos["x"] = c.X;
                jsonPos["y"] = c.Y;
                result.AppendValue(i.second.SerializeToJson(&jsonPos));
            }
        }
        return result;
    }

    bool DeserializeFromProto(const NRTYGeometry::TPolyLine& info) {
        Coords.clear();
        for (ui32 i = 0; i < info.CoordsSize(); ++i) {
            C coord;
            if (!coord.Deserialize(info.GetCoords(i)))
                return false;
            Coords.push_back(coord);
        }
        RebuildStatic();
        return true;
    }

    class TLineInterval {
    private:
        TPosition Start;
        TPosition Finish;

    public:
        TLineInterval() = default;
        TLineInterval(const TPosition& start, const TPosition& finish)
            : Start(start)
            , Finish(finish)
        {
        }

        Y_FORCE_INLINE bool Finished() const {
            return Start.Defined() && Finish.Defined();
        }

        TLineInterval SwapPoints() const {
            TLineInterval result;
            result.Start = Finish;
            result.Finish = Start;
            return result;
        }

        TLineInterval Reverse(const TPolyLine& line) const {
            TLineInterval result;
            result.Start = Start.Reverse(line);
            result.Finish = Finish.Reverse(line);
            return result;
        }

        bool Cross(const TLineInterval& item, double* distanceCross, TLineInterval* intervalCross) const {
            TPosition min = Start;
            TPosition max = Finish;
            TPosition minItem = item.Start;
            TPosition maxItem = item.Finish;
            if (Start > Finish) {
                std::swap(min, max);
            }
            if (item.Start > item.Finish) {
                std::swap(minItem, maxItem);
            }
            min = ::Max<TPosition>(min, minItem);
            max = ::Min<TPosition>(max, maxItem);
            if (min <= max) {
                if (distanceCross)
                    *distanceCross = max.GetDistance() - min.GetDistance();
                if (intervalCross) {
                    *intervalCross = TLineInterval(min, max);
                }
                return true;
            }
            return false;
        }

        Y_FORCE_INLINE void AddPosition(const TPosition& pos) {
            if (!Start.Defined()) {
                Start = pos;
            } else {
                Finish = pos;
            }
        }

        bool IsForward() const {
            CHECK_WITH_LOG(Finish.Defined() && Start.Defined());
            return Finish.GetDistance() > Start.GetDistance();
        }

        Y_FORCE_INLINE const TPosition& GetFirst() const {
            CHECK_WITH_LOG(Start.Defined());
            return Start;
        }

        Y_FORCE_INLINE const TPosition& GetFinish() const {
            CHECK_WITH_LOG(Finish.Defined());
            return Finish;
        }

        Y_FORCE_INLINE const TPosition& GetFirstFwd() const {
            CHECK_WITH_LOG(Start.Defined());
            return IsForward() ? Start : Finish;
        }

        Y_FORCE_INLINE const TPosition& GetFinishFwd() const {
            CHECK_WITH_LOG(Finish.Defined());
            return IsForward() ? Finish : Start;
        }

        const TPosition* Last() const {
            if (Finish.Defined())
                return &Finish;
            if (Start.Defined())
                return &Start;
            return nullptr;
        }

        Y_FORCE_INLINE void Print() const {
            DEBUG_LOG << ToString() << Endl;
        }

        Y_FORCE_INLINE TString ToString() const {
            TStringStream ss;
            ss << Start.GetDistance() << " -> " << Finish.GetDistance() << "/" << Finish.GetDistance() - Start.GetDistance();
            return ss.Str();
        }

        Y_FORCE_INLINE double GetLengthVector() const {
            return Finish.GetDistance() - Start.GetDistance();
        }

        Y_FORCE_INLINE double GetLength() const {
            return Abs(GetLengthVector());
        }

        Y_FORCE_INLINE bool Merge(const TLineInterval& interval) {
            TPosition minFinish = Finish;
            TPosition maxFinish = interval.Finish;
            if (Finish > interval.Finish) {
                std::swap(minFinish, maxFinish);
            }

            TPosition minStart = Start;
            TPosition maxStart = interval.Start;
            if (Start > interval.Start) {
                std::swap(minStart, maxStart);
            }

            if (maxStart <= minFinish) {
                Finish = maxFinish;
                Start = minStart;
                return true;
            }
            return false;
        }

        bool Contain(const TPosition& pos) const {
            if (Finish > Start) {
                return Start <= pos && pos <= Finish;
            } else {
                return Finish <= pos && pos <= Start;
            }
        }
    };

    Y_FORCE_INLINE int SignEps(const double value, const double precision) const {
        if (value < -precision)
            return -1;
        if (value > precision)
            return 1;
        return 0;
    }

    TVector<TPosition> GetCornerPositions(const TVector<TPosition>& poss) const {
        TVector<TPosition> result;
        int direction = 0;
        result.push_back(poss.front());
        for (ui32 i = 1; i < poss.size(); ++i) {
            int directionCurrent = SignEps(poss[i].GetDistance() - poss[i - 1].GetDistance(), 1e-5);
            if ((direction * directionCurrent <= 0 && directionCurrent) || (i == poss.size() - 1)) {
                result.push_back(poss[i]);
                direction = directionCurrent;
            }
        }
        return result;
    }

    TVector<TCoord> GetSegmentCoords(const TVector<TPosition>& poss) const {
        TVector<TCoord> result;
        TVector<TPosition> corners = GetCornerPositions(poss);
        if (corners.size() == 1) {
            return { GetCoord(corners.front()) };
        } else {
            for (ui32 i = 0; i + 1 < corners.size(); ++i) {
                TVector<TCoord> segment = GetSegmentCoords(TLineInterval(corners[i], corners[i + 1]));
                result.insert(result.end(), segment.begin(), segment.end());
            }
        }
        return result;
    }

    TVector<TCoord> GetSegmentCoords(const TLineInterval& interval) const {
        return GetSegmentCoords(interval.GetFirst(), interval.GetFinish());
    }

    TVector<TCoord> GetSegmentCoordsDirect(const TPosition& from, const TPosition& to, const double precision = 1e-3) const {
        TVector<TCoord> result;

        CHECK_WITH_LOG(to.GetDistance() >= from.GetDistance());

        result.push_back(GetCoord(from, precision));
        for (ui32 i = from.GetIndex() + 1; i <= to.GetIndex(); ++i) {
            result.push_back(Coords[i]);
        }
        if (!IsExactPosition(to)) {
            result.push_back(GetCoord(to, precision));
        }
        return result;
    }

    TVector<TCoord> GetSegmentCoords(const TPosition& from, const TPosition& to, const double precision = 1e-3) const {

        if (Abs(to.GetDistance() - from.GetDistance()) < 1e-5) {
            TVector<TCoord> result;
            result.push_back(GetCoord(from, precision));
            result.push_back(result.back());
            return result;
        }

        if (to.GetDistance() >= from.GetDistance()) {
            return GetSegmentCoordsDirect(from, to, precision);
        } else {
            TVector<TCoord> result = GetSegmentCoordsDirect(to, from, precision);
            std::reverse(result.begin(), result.end());
            return result;
        }
    }

    TVector<TCoord> GetSegmentCoordsCircular(const TPosition& from, const TPosition& to, const double precision = 1e-3) const {

        TVector<TCoord> result;
        if (Abs(to.GetDistance() - from.GetDistance()) < 1e-5) {
            result.push_back(GetCoord(from, precision));
            result.push_back(result.back());
            return result;
        }

        if (to.GetDistance() >= from.GetDistance()) {
            result = GetSegmentCoordsDirect(from, to, precision);
        } else if (IsArea()) {
            result = GetSegmentCoordsDirect(from, End(), precision);
            auto v = GetSegmentCoordsDirect(Begin(), to, precision);
            result.insert(result.end(), v.begin(), v.end());
        }
        return result;
    }

    TPolyLine() = default;

    TPolyLine(const TVector<TCoord>& coords)
        : Coords(coords) {
        RebuildStatic();
    }

    TPolyLine(TVector<TCoord>&& coords)
        : Coords(std::move(coords)) {
        RebuildStatic();
    }

    TPolyLine(const TCoord& coord)
        : Coords(1, coord)
    {
        RebuildStatic();
    }

    static TPolyLine FromRect(const TRect& rect) {
        TVector<TCoord> coords = {rect.Min, TCoord(rect.Min.X + rect.GetDX(), rect.Min.Y), rect.Max, TCoord(rect.Max.X - rect.GetDX(), rect.Max.Y)};
        return TPolyLine(coords);
    }

    static TPolyLine GetConvexEnvelope(const TVector<TCoord>& coords) {
        struct TLeftSorter {
            TLeftSorter(const TCoord& start)
                : Start(start) {}

            bool operator()(const TCoord& c1, const TCoord& c2) const {
                TCoordType product = Start.GetVectorProduct(c1, c2);
                if (product == 0) {
                    return c1.X == c2.X ? c1.Y < c2.Y : c1.X < c2.X;
                }
                return product < 0;
            }

        private:
            const TCoord& Start;
        };

        if (coords.empty())
            return TPolyLine();

        ui32 min = 0;
        for (ui32 i = 0; i < coords.size(); ++i) {
            if (coords[i].X < coords[min].X) {
                min = i;
            }
        }

        TVector<TCoord> sorted;
        for (ui32 i = 0; i < coords.size(); ++i) {
            if (i != min) {
                sorted.push_back(coords[i]);
            }
        }

        Sort(sorted.begin(), sorted.end(), TLeftSorter(coords[min]));

        TVector<TCoord> result;
        result.push_back(coords[min]);
        for (auto&& vert : sorted) {
            if (vert.GetLengthTo(result.back()) < 0.1) {
                continue;
            }
            result.push_back(vert);
        }

        if (result.size() <= 3) {
            result.push_back(result[0]);
            return TPolyLine(result);
        }

        TVector<TCoord> convex;
        convex.push_back(result[0]);
        convex.push_back(result[1]);
        for (ui32 i = 2; i < result.size(); ++i) {
            while (convex[convex.size() - 2].GetVectorProduct(convex.back(), result[i]) > 0) {
                convex.pop_back();
                CHECK_WITH_LOG(convex.size() >= 2);
            }
            convex.push_back(result[i]);
        }

        convex.push_back(convex[0]);
        return TPolyLine(convex);
    }

    bool GetNormal(const TPosition& pos, const double length, TCoord& result) const {
        if (Length < 1e-5) {
            return false;
        }
        if (!IsExactPosition(pos) && pos.GetIndex() != Size() - 1 && GetPosition(pos.GetIndex() + 1).GetDistance() - pos.GetDistance() > 1e-5) {
            CHECK_WITH_LOG(TCoord::BuildNormal(Coords[pos.GetIndex()], Coords[pos.GetIndex() + 1], length, result));
            return true;
        }

        TCoord vFront(0, 0);
        for (ui32 i = pos.GetIndex(); i < Size() - 1; ++i) {
            if (GetPosition(i + 1).GetDistance() - GetPosition(i).GetDistance() > 1e-5) {
                CHECK_WITH_LOG(TCoord::BuildNormal(Coords[i], Coords[i + 1], 1, vFront));
                break;
            }
        }
        TCoord vBack(0, 0);
        for (ui32 i = pos.GetIndex(); i >= 1; ++i) {
            if (GetPosition(i).GetDistance() - GetPosition(i - 1).GetDistance() > 1e-5) {
                CHECK_WITH_LOG(TCoord::BuildNormal(Coords[i - 1], Coords[i], 1, vBack));
                break;
            }
        }

        if (GetCoord(pos).BuildMoved(vFront + vBack, length, result)) {
            result = result - GetCoord(pos);
        } else {
            result = TCoord(0, 0);
        }

        return true;

    }

    void RebuildStatic() {
        Length = 0;
        TVector<double> lengthsCache = TVector<double>(Coords.size(), 0);
        PositionsFraction.resize(Coords.size(), 0);
        CHECK_WITH_LOG(Max<TIdxType>() >= Coords.size());
        if (Coords.size()) {
            TRect rect;
            LinkedRect.Link(Coords, 0, rect);
            for (ui32 i = 0; i < Coords.size(); ++i) {
                lengthsCache[i] = Length;
                LinkedRect.Link(Coords, i, rect);
                if (i != Coords.size() - 1) {
                    Length += Coords[i].GetLengthTo(Coords[i + 1]);
                }
            }
        }
        if (Length > 1e-5) {
            const double normMult = Max<ui32>() / Length;
            for (ui32 i = 0; i < lengthsCache.size(); ++i) {
                PositionsFraction[i] = (ui32)(normMult * lengthsCache[i]);
            }
        }
    }

    const TVector<TCoord>& GetCoords() const {
        return Coords;
    }

    TCoord GetCoordByLength(const double distance) const {
        TPosition pos;
        if (!GetPosition(distance, &pos)) {
            pos = End();
        }
        return GetCoord(pos);
    }

    TCoord GetCoord(const TPosition& pos, const double precision = 1e-3) const {
        CHECK_WITH_LOG(pos.GetIndex() < Coords.size());
        if (pos.GetIndex() == Coords.size() - 1) {
            return Coords.back();
        } else {
            auto c1 = Coords[pos.GetIndex()];
            auto c2 = Coords[pos.GetIndex() + 1];
            ASSERT_WITH_LOG(GetPosition(pos.GetIndex()).GetDistance() <= pos.GetDistance() + precision) << GetPosition(pos.GetIndex()).GetDistance() << " / " << pos.GetDistance();
            ASSERT_WITH_LOG(GetPosition(pos.GetIndex() + 1).GetDistance() >= pos.GetDistance() - precision) << GetPosition(pos.GetIndex() + 1).GetDistance() << " / " << pos.GetDistance();
            double d = pos.GetDistance() - GetPosition(pos.GetIndex()).GetDistance();
            double l = GetPosition(pos.GetIndex() + 1).GetDistance() - GetPosition(pos.GetIndex()).GetDistance();
            if (l > 1e-10) {
                const double alpha = ::Min<double>(d / l, 1.0);
                return c1 * (1 - alpha) + c2 * alpha;
            } else {
                return c1;
            }
        }
    }

    bool GetPosition(const double dist, TPosition* result, const double precision = 1e-3) const {
        double lPred = GetPositionLength(0);
        for (ui32 i = 1; i < Coords.size(); ++i) {
            double l = GetPositionLength(i);
            if (l >= dist || (i == Coords.size() - 1 && l + precision >= dist)) {
                TPosition pos(lPred, i - 1);
                if (result)
                    *result = pos.MoveForward(dist - pos.GetDistance());
                return true;
            }
            lPred = l;
        }
        return false;
    }

    Y_FORCE_INLINE ui32 Size() const {
        return Coords.size();
    }

    Y_FORCE_INLINE bool GetRect(TRect& result) const {
        if (!Coords.size()) {
            return false;
        }
        result = LinkedRect.GetRect(Coords);
        return true;
    }

    Y_FORCE_INLINE TRect GetRectSafe() const {
        return LinkedRect.GetRect(Coords);
    }

    bool MatchSimple(const C& c, const double delta, TPosition& result, TCoord* coordResult = nullptr, double* resLength = nullptr) const {
        return Project(c, result, coordResult, resLength, &delta);
    }

    TPosition GluePosition(const TPosition& pos, const double delta) const {
        if (pos.GetIndex() == Size() - 1)
            return pos;
        if (IsExactPosition(pos, delta)) {
            return GetPosition(pos.GetIndex());
        } else {
            TPosition next = GetPosition(pos.GetIndex() + 1);
            if (Abs(next.GetDistance() - pos.GetDistance()) < delta)
                return next;
        }
        return pos;
    }

    bool MatchGlue(const C& c, const double delta, TPosition& result, TCoord* coordResult = nullptr) const {
        bool resultFlag = Project(c, result, coordResult, nullptr, &delta);
        if (resultFlag) {
            CHECK_WITH_LOG((Size() == 1) || (result.GetIndex() < Size() - 1));
            result = GluePosition(result, delta);
        }
        return resultFlag;
    }

    struct TProjectInfo {
        TPosition Position;
        TCoord Coord;
        double Distance;
        bool IsExternal;
        Y_FORCE_INLINE bool operator<(const TProjectInfo& item) const {
            return Distance < item.Distance;
        }
    };

    bool Project(const C& c, TProjectInfo& result, const double* startDelta = nullptr) const {
        return Project(c, result.Position, &result.Coord, &result.Distance, startDelta, &result.IsExternal);
    }

    EMutualPosition GetMutualPosition(const C& c, const TPosition* helpPosition = nullptr) const {
        TPosition pos;
        if (Coords.size() <= 1)
            return EMutualPosition::mpCantDetect;
        if (helpPosition) {
            pos = *helpPosition;
        } else {
            double distance = 0;
            if (!Project(c, pos, nullptr, &distance))
                return EMutualPosition::mpCantDetect;
            if (Abs(distance) < 1e-5)
                return EMutualPosition::mpSame;
        }
        const C* v0 = nullptr;
        const C* v1 = nullptr;
        if (pos.GetIndex() == Coords.size() - 1) {
            v0 = &Coords[pos.GetIndex() - 1];
            v1 = &Coords[pos.GetIndex()];
        } else {
            v0 = &Coords[pos.GetIndex()];
            v1 = &Coords[pos.GetIndex() + 1];
        }
        return v0->GetMutualPosition(*v1, c);
    }

    bool Project(const C& c, TPosition& result, TCoord* coordResult = nullptr, double* coordDistance = nullptr, const double* startDelta = nullptr, bool* isExternal = nullptr, const ui32* fromExt = nullptr, const ui32* toExt = nullptr) const {
        if (!Coords.size())
            return false;
        double minLength = startDelta ? (*startDelta + 1e-5) : Max<ui32>();
        typename C::TCoordType dx = c.MakeDXFromDistance(minLength);
        typename C::TCoordType dy = c.MakeDYFromDistance(minLength);
        if (startDelta) {
            TRect rect;
            CHECK_WITH_LOG(GetRect(rect));
            rect.GrowDXDY(dx * 1.05, dy * 1.05);
            if (!rect.Contain(c))
                return false;
        }

        double cLength;
        C cResult;
        if (isExternal) {
            *isExternal = true;
        }
        if (Coords.size() == 1) {
            cResult = Coords[0];
            minLength = cResult.GetLengthTo(c);
            result = Begin();
        } else {
            ui32 from = fromExt ? *fromExt : 0;
            ui32 to = toExt ? *toExt : Coords.size() - 1;
            for (ui32 i = from; i < to; ++i) {
                const auto& c1 = Coords[i];
                const auto& c2 = Coords[i + 1];

                if (c1.X > c2.X) {
                    if (c1.X + dx < c.X || c2.X - dx > c.X)
                        continue;
                } else if (c2.X + dx < c.X || c1.X - dx > c.X) {
                    continue;
                }

                if (c1.Y > c2.Y) {
                    if (c1.Y + dy < c.Y || c2.Y - dy > c.Y)
                        continue;
                } else if (c2.Y + dy < c.Y || c1.Y - dy > c.Y) {
                    continue;
                }

                double alpha;
                C cProj = c.ProjectTo(c1, c2, &cLength, &alpha);
                if (cLength <= minLength) {
                    if (isExternal) {
                        *isExternal = (alpha > 1 && i == Coords.size() - 2) || (alpha < 0 && i == 0);
                    }
                    minLength = cLength;
                    cResult = cProj;
                    double dist = GetPosition(i).GetDistance();
                    double lDistance = cProj.GetLengthTo(c1);
                    result = TPosition(dist + lDistance, i);
                    dx = c.MakeDXFromDistance(minLength);
                    dy = c.MakeDYFromDistance(minLength);
                }
            }
        }
        if (coordResult)
            *coordResult = cResult;
        if (coordDistance) {
            *coordDistance = minLength;
        }
        return !startDelta ? true : (*startDelta >= minLength);
    }

    bool CloseArea(const double precision = 1e-5) {
        if (Coords.size() && Coords.front().GetLengthTo(Coords.back()) > precision) {
            Coords.push_back(Coords.front());
            RebuildStatic();
        }
    }

    bool IsArea() const {
        return (Coords.size() > 2) && (Coords.front().GetLengthTo(Coords.back()) < 1e-5) && (GetLength() > 1e-5);
    }

    bool IsStart(const TPosition& pos, double delta) const {
        return pos.GetDistance() < delta;
    }

    bool IsFinish(const TPosition& pos, double delta) const {
        return Abs<double>(GetLength() - pos.GetDistance()) < delta;
    }

    static void InflateCoordsImpl(TVector<TCoord>& coords, double delta, const bool circle) {
        TVector<std::pair<TCoord, TCoord>> normals(coords.size());

        {
            ui32 counterExists = 0;
            ui32 i = 0;
            ui32 iExit = Max<ui32>();
            TCoord predNormal;
            while (counterExists != 2) {
                ui32 iPred = (i ? i : coords.size()) - 1;
                if (TCoord::BuildNormal(coords[iPred], coords[i], 1, normals[i].first)) {
                    if (iExit == Max<ui32>())
                        iExit = i;
                    predNormal = normals[i].first;
                } else {
                    normals[i].first = predNormal;
                }
                if (i == iExit)
                    ++counterExists;
                i = (i == coords.size() - 1) ? 0 : (i + 1);
            }
        }

        {
            ui32 counterExists = 0;
            ui32 i = 0;
            ui32 iExit = Max<ui32>();
            TCoord predNormal;
            while (counterExists != 2) {
                ui32 iNext = (i == coords.size() - 1) ? 0 : (i + 1);
                if (TCoord::BuildNormal(coords[i], coords[iNext], 1, normals[i].second)) {
                    if (iExit == Max<ui32>())
                        iExit = i;
                    predNormal = normals[i].second;
                } else {
                    normals[i].second = predNormal;
                }
                if (i == iExit)
                    ++counterExists;
                i = (i ? i : coords.size()) - 1;
            }
        }

        double orientMarker = 0;
        if (circle) {
            for (ui32 i = 0; i < coords.size() - 1; ++i) {
                orientMarker += (coords[i + 1].Y + coords[i].Y) * (coords[i + 1].X - coords[i].X);
            }
        } else {
            orientMarker = 1;
        }

        double kff = 1;
        if (orientMarker < 0) {
            kff = -1;
        }

        for (ui32 i = 0; i < coords.size(); ++i) {
            TCoord v;
            if (circle) {
                v = normals[i].first + normals[i].second;
            } else {
                if (i == 0) {
                    v = normals[i].second;
                } else if (i == coords.size() - 1) {
                    v = normals[i].first;
                } else {
                    v = normals[i].first + normals[i].second;
                }
            }

            TCoord c = coords[i] + v;
            double d = c.GetLengthTo(coords[i]);
            if (d > 1e-5) {
                coords[i] = coords[i] + (v)* kff * delta / d;
            }
        }
    }

    bool InflateImpl(const double delta, const bool circle) {
        if (GetLength() < 1e-5) {
            return false;
        }

        InflateCoordsImpl(Coords, delta, circle);

        RebuildStatic();
        return true;
    }

    bool Inflate(const double delta) {
        if (Coords.size() < 3)
            return false;
        if (Coords.front().GetLengthTo(Coords.back()) > 1e-5) {
            return false;
        }

        return InflateImpl(delta, true);
    }

    bool InflateLine(const double delta) {
        if (Coords.size() < 2)
            return false;

        return InflateImpl(delta, false);
    }

    bool CutSphere(const C& c, const double r, TLineInterval& interval, const double lengthPrecision) const {
        if (!Size())
            return false;

        if (Size() == 1) {
            interval = TLineInterval(Begin(), End());
            return Coords[0].GetLengthTo(c) <= r;
        }

        TProjectInfo info;
        if (!Project(c, info, &r))
            return false;
        return CutSphere(info, c, r, interval, lengthPrecision);
    }

    bool CutSphere(const TProjectInfo& pInfo, const TGeoCoord& c, const double r, TLineInterval& interval, const double lengthPrecision) const {

        i32 indexTo = pInfo.Position.GetIndex() + 1;
        i32 indexFrom = pInfo.Position.GetIndex();
        for (; (ui32)indexTo < Size(); ++indexTo) {
            if (Coords[indexTo].GetLengthTo(c) > r)
                break;
        }

        for (; (indexFrom >= 0); --indexFrom) {
            if (Coords[indexFrom].GetLengthTo(c) > r)
                break;
        }

        TPosition posTo;
        TPosition posFrom;
        TCoord c1;
        TCoord c2;
        if (indexFrom == -1) {
            posFrom = Begin();
        } else {
            ui32 countSolutions = c.CrossSphere(r, Coords[indexFrom], Coords[indexFrom + 1], c1, c2, lengthPrecision);
            if (!countSolutions) {
                double distInternal = pInfo.Distance;
                if (indexFrom + 1 != indexTo)
                    distInternal = Coords[indexFrom + 1].GetLengthTo(c);

                Y_ASSERT(r > 1000 || (::Abs(distInternal - r) < lengthPrecision));
                c1 = pInfo.Coord;
            }
            posFrom = GetPosition(indexFrom);
            posFrom = posFrom.MoveForward(Coords[indexFrom].GetLengthTo(c1));
        }

        if ((ui32)indexTo == Size()) {
            posTo = End();
        } else {
            ui32 countSolutions = c.CrossSphere(r, Coords[indexTo - 1], Coords[indexTo], c1, c2, lengthPrecision);
            if (!countSolutions) {
                double distInternal = pInfo.Distance;
                if (indexTo - 1 != indexFrom)
                    distInternal = Coords[indexTo - 1].GetLengthTo(c);
                Y_ASSERT(r > 1000 || (::Abs(distInternal - r) < lengthPrecision));
                c2 = pInfo.Coord;
            }
            posTo = GetPosition(indexTo - 1);
            posTo = posTo.MoveForward(Coords[indexTo - 1].GetLengthTo(c2));
        }

        interval = TLineInterval(posFrom, posTo);

        return true;
    }

    int GetDirectionVector(const C& c, const C& v, double delta) const {
        TPosition resultBase;
        TPosition resultMoved;
        TPosition resultMovedBack;
        TCoord matchCoord;
        double vLength = v.SimpleLength();
        if (vLength < 1e-5)
            return 0;
        if (!Project(c, resultBase, &matchCoord, nullptr, &delta))
            return 0;

        CHECK_WITH_LOG(Project(matchCoord + v / vLength * 0.0001, resultMoved));
        CHECK_WITH_LOG(Project(matchCoord - v / vLength * 0.0001, resultMovedBack));
        if (resultMoved.GetDistance() > resultMovedBack.GetDistance())
            return 1;
        if (resultMoved.GetDistance() < resultMovedBack.GetDistance())
            return -1;
        return 0;
    }

    const TCoord& FirstCoord() const {
        CHECK_WITH_LOG(Coords.size());
        return Coords[0];
    }

    const TCoord& LastCoord() const {
        CHECK_WITH_LOG(Coords.size());
        return Coords.back();
    }

    Y_FORCE_INLINE const TCoord& operator[](const ui32 index) const {
        CHECK_WITH_LOG(index < Coords.size());
        return Coords[index];
    }

    Y_FORCE_INLINE TPosition GetPosition(const ui32 index) const {
        CHECK_WITH_LOG(Coords.size() > index);
        CHECK_WITH_LOG(PositionsFraction.size() == Coords.size());
        return TPosition(GetPositionLength(index), index);
    }

    struct TLinkedPosition {
    private:
        bool IsLinked = false;
        TPosition Link;
        TPosition Main;

    public:
        TLinkedPosition() = default;
        TLinkedPosition(const TPosition& pos)
            : Main(pos)
        {
        }

        Y_FORCE_INLINE bool operator<(const TLinkedPosition& item) const {
            return Main < item.Main;
        }

        Y_FORCE_INLINE bool operator>(const TLinkedPosition& item) const {
            return Main > item.Main;
        }

        Y_FORCE_INLINE void MakeLink(TLinkedPosition& pos) {
            CHECK_WITH_LOG(!IsLinked && !pos.IsLinked);
            IsLinked = true;
            Link = pos.Main;
            pos.IsLinked = true;
            pos.Link = Main;
        }

        Y_FORCE_INLINE const TPosition& GetLink() const {
            CHECK_WITH_LOG(IsLinked);
            return Link;
        }

        Y_FORCE_INLINE const TPosition& GetMain() const {
            return Main;
        }

        Y_FORCE_INLINE bool Linked() const {
            return IsLinked;
        }
    };

    Y_FORCE_INLINE double GetLength() const {
        return Length;
    }

    Y_FORCE_INLINE double GetLength(const ui32 from, const ui32 to) const {
        return Abs(GetPosition(to).GetDistance() - GetPosition(from).GetDistance());
    }

    Y_FORCE_INLINE TPosition Begin() const {
        return TPosition(0, 0);
    }

    Y_FORCE_INLINE TPosition End() const {
        return TPosition(GetLength(), Size() - 1);
    }

    TString ToString() const {
        TStringStream result;
        for (auto&& i : Coords) {
            result << i.ToString() << " ";
        }
        result << ";rect=" << LinkedRect.GetRect(Coords).ToString();
        result << ";length=" << GetLength();
        return result.Str();
    }

    TString ToStringReverse() const {
        TStringStream result;
        for (i32 i = Coords.size() - 1; i >= 0; --i) {
            result << Coords[i].ToString() << " ";
        }
        result << ";rect=" << LinkedRect.GetRect(Coords).ToString();
        result << ";length=" << GetLength();
        return result.Str();
    }

    bool GetLengthTo(const TCoord& coord, double& result) const {
        TPosition pos;
        return Project(coord, pos, nullptr, &result);
    }

    TSet<TPosition> Cross(const TPolyLine& line) const {
        TSet<TPosition> result;
        for (ui32 i = 0; i < line.Size() - 1; ++i) {
            for (ui32 j = 0; j < Size() - 1; ++j) {
                TCoord cRes;
                if (TCoord::Cross(line[i], line[i + 1], Coords[j], Coords[j + 1], &cRes)) {
                    result.insert(line.GetPosition(i).MoveForward(cRes.GetLengthTo(line[i])));
                }
            }
        }
        return result;
    }

    bool Cross(const TPolyLine& line, TVector<TPosition>* selfPositions, TVector<TPosition>* linePositions) const {
        if (selfPositions) {
            selfPositions->clear();
        }
        if (linePositions) {
            linePositions->clear();
        }
        if (!line.Size() || !Size()) {
            return false;
        }
        if (!line.GetRectSafe().Cross(GetRectSafe())) {
            return false;
        }
        for (ui32 i = 0; i < line.Size() - 1; ++i) {
            for (ui32 j = 0; j < Size() - 1; ++j) {
                TCoord cRes;
                if (TCoord::Cross(line[i], line[i + 1], Coords[j], Coords[j + 1], &cRes)) {
                    if (linePositions) {
                        linePositions->emplace_back(line.GetPosition(i).MoveForward(cRes.GetLengthTo(line[i])));
                    }
                    if (selfPositions) {
                        selfPositions->emplace_back(GetPosition(j).MoveForward(cRes.GetLengthTo(Coords[j])));
                    }
                    if (!selfPositions && !linePositions) {
                        return true;
                    }
                }
            }
        }
        if (linePositions) {
            return linePositions->size();
        }
        if (selfPositions) {
            return selfPositions->size();
        }
        return false;
    }

    bool CrossCheck(const TCoord& c1, const TCoord& c2, TCoord* result = nullptr) const {
        const TRect rect(c1, c2);
        if (!TRect(c1, c2).Cross(GetRectSafe()))
            return false;
        for (ui32 j = 0; j < Size() - 1; ++j) {
            if (TCoord::Cross(c1, c2, Coords[j], Coords[j + 1], result)) {
                return true;
            }
        }
        return false;
    }

    bool GetPosition(const ui64 t, const TVector<ui64>& externalParametrization, TPosition* result) {
        CHECK_WITH_LOG(externalParametrization.size() == Size());
        if (Size() == 1)
            return false;
        if (*externalParametrization.begin() > t)
            return false;
        if (*externalParametrization.rbegin() < t)
            return false;
        auto it = UpperBound(externalParametrization.begin(), externalParametrization.end(), t);
        if (it == externalParametrization.begin()) {
            return false;
        }

        if (it == externalParametrization.end()) {
            *result = GetPosition(Size() - 1);
            return true;
        }

        --it;

        CHECK_WITH_LOG(*it <= t);

        if (result) {
            ui32 id = it - externalParametrization.begin();
            CHECK_WITH_LOG(externalParametrization[id + 1] >= t);
            CHECK_WITH_LOG(externalParametrization[id] <= t);
            *result = GetPosition(id);
            if (externalParametrization[id + 1] == externalParametrization[id]) {
                return true;
            }
            double dl = GetPosition(id + 1).GetDistance() - GetPosition(id).GetDistance();
            i64 dt = externalParametrization[id + 1] - externalParametrization[id];
            CHECK_WITH_LOG(dt > 0);
            result->MoveForward(dl / dt * (t - externalParametrization[id]));
        }

        return true;
    }

    bool GetTangent(const ui32 index, TCoord& result) const {
        VERIFY_WITH_LOG(index < Size(), "%u < %u", index, Size());
        if (Size() < 2)
            return false;

        ui32 indexPred = index;
        ui32 indexNext = index;
        while (true) {
            result = Coords[indexNext] - Coords[indexPred];
            if (Coords[indexNext].GetLengthTo(Coords[indexPred]) > 1e-5) {
                return true;
            }
            if (indexNext < Coords.size() - 1) {
                ++indexNext;
                continue;
            }
            if (indexPred > 0) {
                --indexPred;
                continue;
            }
            break;
        }

        return false;
    }

    double GetArea() const {
        return TCoord::GetArea(Coords);
    }

    bool IsPointInternal(const TCoord& coord) const {
        if (Coords.size() < 3)
            return false;
        if (!GetRectSafe().Contain(coord)) {
            return false;
        }

        if (Coords[0].GetLengthTo(Coords.back()) > 1e-5)
            return false;

        ui32 counter = 0;
        for (ui32 i = 0; i < Coords.size() - 1; ++i) {
            TCoordType dy0 = Coords[i].Y - coord.Y;
            TCoordType dy1 = Coords[i + 1].Y - coord.Y;
            if (Coords[i].X < coord.X && Coords[i + 1].X < coord.X)
                continue;
            if (dy1 == 0 && dy0 == 0)
                continue;
            if (dy0 * dy1 > 0 || (dy1 == 0 && dy0 < 0) || (dy0 == 0 && dy1 < 0))
                continue;
            if (Abs(Coords[i + 1].Y - Coords[i].Y) < 1e-10)
                continue;
            double kff = (coord.Y - Coords[i].Y) / (Coords[i + 1].Y - Coords[i].Y);
            if (kff < 0 || kff > 1)
                continue;
            if ((Coords[i + 1].X - Coords[i].X) * kff <= coord.X - Coords[i].X + 1e-10)
                continue;
            ++counter;
        }
        return counter % 2;
    }

    class TPairPosition {
    private:
        ui32 Id = 0;
        TPosition SelfPosition;
        TPosition ExtPosition;
    public:
        TPairPosition(const ui32 idx, const TPosition& selfPosition, const TPosition& extPosition)
            : Id(idx)
            , SelfPosition(selfPosition)
            , ExtPosition(extPosition)
        {

        }

        ui32 GetId() const {
            return Id;
        }

        const TPosition& GetSelfPosition() const {
            return SelfPosition;
        }

        const TPosition& GetExtPosition() const {
            return ExtPosition;
        }
    };

    static TVector<TCoord> AddLines(const TVector<TCoord>& l1, const bool fwd1, const TVector<TCoord>& l2, const bool fwd2) {
        if (!l1.size()) {
            if (fwd2) {
                return l2;
            } else {
                return TVector<TCoord>(l2.rbegin(), l2.rend());
            }
        }
        if (!l2.size()) {
            if (fwd1) {
                return l1;
            } else {
                return TVector<TCoord>(l1.rbegin(), l1.rend());
            }
        }
        TVector<TCoord> result;
        if (fwd1) {
            result = l1;
        } else {
            result = TVector<TCoord>(l1.rbegin(), l1.rend());
        }
        if (fwd2) {
            result.insert(result.end(), l2.begin(), l2.end());
        } else {
            result.insert(result.end(), l2.rbegin(), l2.rend());
        }
        return result;
    }

    class TResultSegment {
    private:
        TPolyLine Line;
        TPairPosition From;
        TPairPosition To;
        bool IsSelf = false;
    public:

        TVector<TCoord> FinalCoords() const {
            TVector<TCoord> result;
            for (auto&& i : Line.GetCoords()) {
                if (result.empty() || result.back().GetLengthTo(i) > 1e-5) {
                    result.emplace_back(i);
                }
            }
            if (result.size()) {
                result.back() = result.front();
            }
            return result;
        }

        const TPolyLine& GetLine() const {
            return Line;
        }

        bool operator < (const TResultSegment& item) const {
            return std::tie(From.GetId(), To.GetId(), IsSelf) < std::tie(item.From.GetId(), item.To.GetId(), IsSelf);
        }

        const TVector<TCoord>& GetCoords() const {
            return Line.GetCoords();
        }

        TString GetHash() const {
            return ::ToString(From.GetId()) + "-" + ::ToString(To.GetId()) + "-" + ::ToString(IsSelf);
        }

        TResultSegment(const TPairPosition& from, const TPairPosition& to, TPolyLine&& line, const bool isSelf)
            : Line(std::move(line))
            , From(from)
            , To(to)
            , IsSelf(isSelf)
        {

        }

        bool IsFinished() const {
            return From.GetId() == To.GetId();
        }

        bool Merge(const TResultSegment& segment) {
            if ((ui64)this == (ui64)(&segment)) {
                return false;
            }
            if (From.GetId() == segment.To.GetId()) {
                From = segment.From;
                Line = TPolyLine(AddLines(segment.Line.GetCoords(), true, Line.GetCoords(), true));
            } else if (To.GetId() == segment.To.GetId()) {
                To = segment.From;
                Line = TPolyLine(AddLines(Line.GetCoords(), true, segment.Line.GetCoords(), false));
            } else if (From.GetId() == segment.From.GetId()) {
                From = segment.To;
                Line = TPolyLine(AddLines(segment.Line.GetCoords(), false, Line.GetCoords(), true));
            } else if (To.GetId() == segment.From.GetId()) {
                To = segment.To;
                Line = TPolyLine(AddLines(Line.GetCoords(), true, segment.Line.GetCoords(), true));
            } else {
                return false;
            }
            return true;
        }
    };

    TVector<TPolyLine> AreasIntersection(const TPolyLine& extArea, const bool internalSelf = true, const bool internalExt = true) const {
        if (!internalSelf && !internalExt) {
            return TVector<TPolyLine>();
        }
        if (!Size() && !extArea.Size()) {
            return {};
        }
        if (!Size()) {
            if (!internalSelf) {
                if (extArea.Size()) {
                    return {extArea};
                } else {
                    return {};
                }
            } else {
                return {};
            }
        }
        if (!extArea.Size()) {
            if (!internalExt) {
                if (Size()) {
                    return {*this};
                } else {
                    return {};
                }
            } else {
                return {};
            }
        }
        TVector<TPosition> selfPositions;
        TVector<TPosition> extPositions;
        Cross(extArea, &selfPositions, &extPositions);
        CHECK_WITH_LOG(selfPositions.size() == extPositions.size());
        TVector<TPairPosition> positions;
        for (ui32 i = 0; i < selfPositions.size(); ++i) {
            positions.emplace_back(i, selfPositions[i], extPositions[i]);
        }

        if (positions.empty()) {
            if (IsPointInternal(extArea.FirstCoord())) {
                if (internalSelf && internalExt) {
                    return {extArea};
                } else if (!internalSelf) {
                    return {};
                } else if (!internalExt) {
                    return {*this};
                }
            } else if (extArea.IsPointInternal(FirstCoord())) {
                if (internalSelf && internalExt) {
                    return {*this};
                } else if (!internalExt) {
                    return {};
                } else if (!internalSelf) {
                    return {extArea};
                }
            } else {
                if (internalSelf && internalExt) {
                    return {};
                } else if (!internalExt) {
                    return {*this};
                } else if (!internalSelf) {
                    return {extArea};
                }
            }
        }

        const auto predSortSelf = [](const TPairPosition& l, const TPairPosition& r) {
            return l.GetSelfPosition().GetDistance() < r.GetSelfPosition().GetDistance();
        };
        std::sort(positions.begin(), positions.end(), predSortSelf);
        TVector<TResultSegment> segments;
        TSet<TString> segmentsChecker;
        double precision = 1e-5;
        TPosition prPos;
        for (ui32 i = 0; i < positions.size(); ++i) {
            const TPosition& from = positions[i].GetSelfPosition();
            const TPosition& to = positions[(i + 1) % positions.size()].GetSelfPosition();
            const TVector<TCoord> coords = GetSegmentCoordsCircular(from, to);
            if (coords.empty()) {
                continue;
            }
            TPolyLine pl(coords);
            const TCoord c = pl.GetCoordByLength(pl.GetLength() * 0.5);
            if ((extArea.IsPointInternal(c)) == internalExt) {
                segments.emplace_back(positions[i], positions[(i + 1) % positions.size()], std::move(pl), true);
                Y_ASSERT(segmentsChecker.emplace(segments.back().GetHash()).second);
            }
        }

        const auto predSortExt = [](const TPairPosition& l, const TPairPosition& r) {
            return l.GetExtPosition().GetDistance() < r.GetExtPosition().GetDistance();
        };
        std::sort(positions.begin(), positions.end(), predSortExt);
        for (ui32 i = 0; i < positions.size(); ++i) {
            const TPosition& from = positions[i].GetExtPosition();
            const TPosition& to = positions[(i + 1) % positions.size()].GetExtPosition();
            const TVector<TCoord> coords = extArea.GetSegmentCoordsCircular(from, to);
            if (coords.empty()) {
                continue;
            }
            TPolyLine pl(coords);
            const TCoord c = pl.GetCoordByLength(pl.GetLength() * 0.5);
            if ((IsPointInternal(c) || Project(c, prPos, nullptr, nullptr, &precision)) == internalSelf) {
                segments.emplace_back(positions[i], positions[(i + 1) % positions.size()], std::move(pl), false);
                Y_ASSERT(segmentsChecker.emplace(segments.back().GetHash()).second);
            }
        }

        bool hasChanges = false;
        do {
            hasChanges = false;
            for (auto i = segments.begin(); i != segments.end(); ++i) {
                for (auto j = segments.begin(); j != segments.end(); ++j) {
                    if (i->Merge(*j)) {
                        hasChanges = true;
                        segments.erase(j);
                        break;
                    }
                }
                if (hasChanges) {
                    break;
                }
            }
        } while (hasChanges);
        TVector<TPolyLine> result;
        for (auto&& i : segments) {
            if (i.IsFinished() && i.GetLine().GetArea() > 1e-5) {
                result.emplace_back(i.FinalCoords());
            }
        }
        return result;
    }

};

using TGeoPolyLine = TPolyLine<TGeoCoord>;
