#pragma once
#include "coord.h"
#include <util/generic/ymath.h>
#include <library/cpp/logger/global/global.h>

template <class TCoord>
struct TRect {

    using TSelf = TRect<TCoord>;
    using TCoordType = typename TCoord::TCoordType;

    TCoord Min;
    TCoord Max;

    TRect() {
    }

    TVector<TCoord> GetCoords() const {
        TVector<TCoord> result;
        result.emplace_back(TCoord(Min.X, Min.Y));
        result.emplace_back(TCoord(Max.X, Min.Y));
        result.emplace_back(TCoord(Max.X, Max.Y));
        result.emplace_back(TCoord(Min.X, Max.Y));
        result.emplace_back(result.front());
        return result;
    }

    TRect(const TCoord& c) {
        Min = Max = c;
    }

    TRect(const TCoord& c, const TVector<TCoord>& coords) {
        Min = Max = c;
        for (auto&& i : coords) {
            ExpandTo(i);
        }
    }

    TRect(const TCoordType x0, const TCoordType y0, const TCoordType x1, const TCoordType y1) {
        Min = TCoord(::Min(x0, x1), ::Min(y0, y1));
        Max = TCoord(::Max(x0, x1), ::Max(y0, y1));
    }

    TRect(const TCoord& c, const TCoordType& delta) {
        Min = TCoord(c.X - delta, c.Y - delta);
        Max = TCoord(c.X + delta, c.Y + delta);
    }

    TRect(const TCoord& c, const TCoordType& dx, const TCoordType& dy) {
        Min = TCoord(c.X - dx, c.Y - dy);
        Max = TCoord(c.X + dx, c.Y + dy);
    }

    TRect(const TCoord& c1, const TCoord& c2) {
        Min = c1;
        Max = c2;
        if (c1.X > c2.X) {
            std::swap(Min.X, Max.X);
        }
        if (c1.Y > c2.Y) {
            std::swap(Min.Y, Max.Y);
        }
    }

    TVector<TSelf> GetAdditionalRects(const TSelf& fullRect) const {
        VERIFY_WITH_LOG(fullRect.ContainLB(*this), "%s, %s", ToString().data(), fullRect.ToString().data());
        TVector<TSelf> result;
        TCoord minCoord = fullRect.Min;
        TCoord maxCoord = fullRect.Max;
        result.push_back(TSelf(minCoord.X, Max.Y, Min.X, maxCoord.Y));
        result.push_back(TSelf(Min.X, Max.Y, Max.X, maxCoord.Y));
        result.push_back(TSelf(Max.X, Max.Y, maxCoord.X, maxCoord.Y));

        result.push_back(TSelf(minCoord.X, Min.Y, Min.X, Max.Y));
        result.push_back(TSelf(Max.X, Min.Y, maxCoord.X, Max.Y));

        result.push_back(TSelf(minCoord.X, minCoord.Y, Min.X, Min.Y));
        result.push_back(TSelf(Min.X, minCoord.Y, Max.X, Min.Y));
        result.push_back(TSelf(Max.X, minCoord.Y, maxCoord.X, Min.Y));
        ui32 delta = 0;
        for (ui32 i = 0; i + delta < result.size();) {
            result[i] = result[i + delta];
            if (TCoord::CalcRectArea(result[i].Min, result[i].Max) < 1e-5) {
                ++delta;
            } else {
                ++i;
            }
        }
        result.resize(result.size() - delta);
        return std::move(result);
    }

    double GetLengthTo(const TSelf& rect) const {
        TCoordType MinMaxX = ::Min<TCoordType>(rect.Max.X, Max.X);
        TCoordType MaxMinX = ::Max<TCoordType>(rect.Min.X, Min.X);
        TCoord c1;
        TCoord c2;
        c1.X = Max.X;
        c2.X = Max.X;
        if (MinMaxX < MaxMinX) {
            c1.X = MinMaxX;
            c2.X = MaxMinX;
        }

        TCoordType MinMaxY = ::Min<TCoordType>(rect.Max.Y, Max.Y);
        TCoordType MaxMinY = ::Max<TCoordType>(rect.Min.Y, Min.Y);
        c1.Y = Max.Y;
        c2.Y = Max.Y;
        if (MinMaxY < MaxMinY) {
            c1.Y = MinMaxY;
            c2.Y = MaxMinY;
        }

        return c1.GetLengthTo(c2);
    }

    TString ToString() const {
        return Min.ToString() + "->" + Max.ToString();
    }

    TSelf GrowMultiple(TCoordType mult) const {
        TCoordType cx = 0.5 * (Min.X + Max.X);
        TCoordType cy = 0.5 * (Min.Y + Max.Y);
        TCoordType dx = Max.X - Min.X;
        TCoordType dy = Max.Y - Min.Y;

        return TSelf(TCoord(cx - dx / 2.0 * mult, cy - dy / 2.0 * mult), TCoord(cx + dx / 2.0 * mult, cy + dy / 2.0 * mult));
    }

    Y_FORCE_INLINE bool ContainLB(const TCoord& c) const {
        return c.X >= Min.X && c.Y >= Min.Y && c.X < Max.X && c.Y < Max.Y;
    }

    Y_FORCE_INLINE bool Contain(const TCoord& c) const {
        return c.X >= Min.X && c.Y >= Min.Y && c.X <= Max.X && c.Y <= Max.Y;
    }

    Y_FORCE_INLINE bool Contain(const TSelf& r) const {
        return r.Min.X >= Min.X && r.Min.Y >= Min.Y && r.Max.X <= Max.X && r.Max.Y <= Max.Y;
    }

    Y_FORCE_INLINE bool ContainLB(const TSelf& r) const {
        if (r.Min.X < Min.X || r.Min.Y < Min.Y)
            return false;

        if (r.Max.X > Max.X || r.Max.Y > Max.Y)
            return false;

        if (r.Max.X == r.Min.X) {
            if (r.Max.Y == r.Min.Y) {
                return r.Max.X < Max.X && r.Max.Y < Max.Y;
            } else {
                return r.Max.X < Max.X;
            }
        } else {
            if (r.Max.Y == r.Min.Y) {
                return r.Max.Y < Max.Y;
            }
        }
        return true;
    }

    Y_FORCE_INLINE bool Cross(const TSelf& rect, TSelf& result) const {
        result.Min.X = ::Max(Min.X, rect.Min.X);
        result.Min.Y = ::Max(Min.Y, rect.Min.Y);
        result.Max.X = ::Min(Max.X, rect.Max.X);
        result.Max.Y = ::Min(Max.Y, rect.Max.Y);

        return (result.Min.X <= result.Max.X && result.Min.Y <= result.Max.Y);
    }

    Y_FORCE_INLINE bool Cross(const TSelf& rect) const {
        return rect.Max.X >= Min.X && Max.X >= rect.Min.X && rect.Max.Y >= Min.Y && Max.Y >= rect.Min.Y;
    }

    Y_FORCE_INLINE bool CrossLB(const TSelf& rect) const {
        bool xCorrect = rect.Max.X > Min.X && Max.X > rect.Min.X;
        if (!xCorrect && rect.Max.X == rect.Min.X) {
            xCorrect = rect.Max.X >= Min.X && Max.X > rect.Min.X;
        }
        if (xCorrect) {
            bool yCorrect = rect.Max.Y > Min.Y && Max.Y > rect.Min.Y;
            if (!yCorrect && rect.Max.Y == rect.Min.Y) {
                yCorrect = rect.Max.Y >= Min.Y && Max.Y > rect.Min.Y;
            }
            return xCorrect && yCorrect;
        }

        return false;
    }

    Y_FORCE_INLINE void ExpandTo(const TSelf& c) {
        if (Min.X > c.Min.X)
            Min.X = c.Min.X;
        if (Min.Y > c.Min.Y)
            Min.Y = c.Min.Y;
        if (Max.X < c.Max.X)
            Max.X = c.Max.X;
        if (Max.Y < c.Max.Y)
            Max.Y = c.Max.Y;
    }

    Y_FORCE_INLINE void ExpandTo(const TCoord& c) {
        if (Min.X > c.X)
            Min.X = c.X;
        if (Min.Y > c.Y)
            Min.Y = c.Y;
        if (Max.X < c.X)
            Max.X = c.X;
        if (Max.Y < c.Y)
            Max.Y = c.Y;
    }

    template <class TCoordLocal>
    Y_FORCE_INLINE void ExpandTo(const TVector<TCoordLocal>& vc) {
        for (auto&& i : vc) {
            ExpandTo(i);
        }
    }

    Y_FORCE_INLINE void Grow(TCoordType delta) {
        Min.X -= delta;
        Min.Y -= delta;
        Max.X += delta;
        Max.Y += delta;
    }

    TRect GetGrowDistance(const double delta) const {
        TRect result = *this;
        result.GrowDistance(delta);
        return std::move(result);
    }

    Y_FORCE_INLINE void GrowDistance(const double delta) {
        Min.X -= Min.MakeDXFromDistance(delta);
        Min.Y -= Min.MakeDYFromDistance(delta);
        Max.X += Max.MakeDXFromDistance(delta);
        Max.Y += Max.MakeDYFromDistance(delta);
    }

    void GrowDXDY(const TCoordType& dx, const TCoordType& dy) {
        Min.X -= dx;
        Max.X += dx;
        Min.Y -= dy;
        Max.Y += dy;
    }

    void GrowDX(TCoordType delta) {
        Min.X -= delta;
        Max.X += delta;
    }

    void GrowDY(TCoordType delta) {
        Min.Y -= delta;
        Max.Y += delta;
    }

    TCoord GetCenter() const {
        return TCoord(0.5 * (Min.X + Max.X), 0.5 * (Min.Y + Max.Y));
    }

    TCoordType GetDX() const {
        return Max.X - Min.X;
    }

    TCoordType GetDY() const {
        return Max.Y - Min.Y;
    }

    TVector<TCoord> GetClipping(const TCoord& p1, const TCoord& p2) const {
        const auto alphaX = [&p1, &p2](TCoordType x)->double {
            return (p1.X - x) / (p1.X - p2.X);
        };

        TVector<TCoord> intersections;

        {
            TCoordType aXMin = alphaX(Min.X);
            if (aXMin >= 0 && aXMin <= 1) {
                TCoord p(Min.X, aXMin * p2.Y + (1 - aXMin) * p1.Y);
                if (Contain(p) && p.Y != Min.Y && p.Y != Max.Y)
                    intersections.push_back(p);
            }
        }

        {
            TCoordType aXMax = alphaX(Max.X);
            if (aXMax >= 0 && aXMax <= 1) {
                TCoord p(Max.X, aXMax * p2.Y + (1 - aXMax) * p1.Y);
                if (Contain(p) && p.Y != Min.Y && p.Y != Max.Y)
                    intersections.push_back(p);
            }
        }

        const auto alphaY = [&p1, &p2](TCoordType y)->double {
            return (p1.Y - y) / (p1.Y - p2.Y);
        };

        {
            TCoordType aYMin = alphaY(Min.Y);
            if (aYMin >= 0 && aYMin <= 1) {
                TCoord p(aYMin * p2.X + (1 - aYMin) * p1.X, Min.Y);
                if (Contain(p))
                    intersections.push_back(p);
            }
        }

        {
            TCoordType aYMax = alphaY(Max.Y);
            if (aYMax >= 0 && aYMax <= 1) {
                TCoord p(aYMax * p2.X + (1 - aYMax) * p1.X, Max.Y);
                if (Contain(p))
                    intersections.push_back(p);
            }
        }

        Y_ASSERT(intersections.size() < 3);
        return intersections;
    }
};

using TGeoRect = TRect<TGeoCoord>;
