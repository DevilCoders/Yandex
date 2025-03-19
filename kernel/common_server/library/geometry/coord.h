#pragma once
#include <util/generic/ymath.h>
#include <util/generic/string.h>
#include <kernel/common_server/library/geometry/protos/geometry.pb.h>
#include <util/string/cast.h>
#include <util/string/strip.h>
#include <util/string/vector.h>
#include <library/cpp/logger/global/global.h>
#include <util/digest/multi.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/string/split.h>
#include <kernel/common_server/util/json_processing.h>
#include <kernel/common_server/library/logging/events.h>

enum class EMutualPosition {
    mpLeft /* "left" */,
    mpSame,
    mpRight /* "right" */,
    mpReverse,
    mpCantDetect
};

template <class T, class TProduct>
struct TBaseCoord {
    using TCoordType = T;
    using TSelf = TProduct;
    T X = 0;
    T Y = 0;

    TBaseCoord() {

    }

    TBaseCoord(T x, T y)
        : X(x)
        , Y(y)
    {

    }

    template <class TAlternative>
    TProduct GetDiffPrecisionCoord() const {
        TAlternative xNew = X;
        TAlternative yNew = Y;
        return TProduct(xNew, yNew);
    }

    bool operator == (const TBaseCoord& item) const {
        return X == item.X && Y == item.Y;
    }

    template <class TCoord, class TStringType>
    Y_WARN_UNUSED_RESULT static bool DeserializeVector(const TStringType& data, TVector<TCoord>& result) {
        return DeserializeVectorImpl(data, result);
    }

    template <class TCoord, class TStringType>
    Y_WARN_UNUSED_RESULT static bool DeserializeVectorByPoints(const TStringType& data, TVector<TCoord>& result) {
        return DeserializeVectorImpl<TCoord, TStringType, ' ', ','>(data, result);
    }

    template <class TCoord, class TStringType, const char delimiterCoords = ' ', const char delimiterPoints = ' '>
    Y_WARN_UNUSED_RESULT static bool DeserializeVectorImpl(const TStringType& data, TVector<TCoord>& result) {
        result.clear();
        TCoord currentValue;
        try {
            if (delimiterPoints != delimiterCoords) {
                for (const auto& t : StringSplitter(data).Split(delimiterPoints)) {
                    auto sb = t.Token();
                    if (!TryFromString<TCoord>(sb, currentValue)) {
                        return false;
                    }
                    result.push_back(currentValue);
                }
            } else {
                ui32 idx = 0;
                for (const auto& t : StringSplitter(data).Split(delimiterPoints)) {
                    auto sb = t.Token();
                    if (sb.empty())
                        continue;
                    if (idx % 2 == 0) {
                        if (!TryFromString<typename TCoord::TCoordType>(sb, currentValue.X)) {
                            return false;
                        }
                    } else if (!TryFromString<typename TCoord::TCoordType>(sb, currentValue.Y)) {
                        return false;
                    }
                    if (++idx % 2 == 0) {
                        result.push_back(currentValue);
                    }
                }
                if (idx % 2) {
                    return false;
                }
            }
        } catch (...) {
            return false;
        }
        return true;
    }

    template <class TCoord>
    static TString SerializeVector(const TVector<TCoord>& data) {
        TString result;
        for (auto&& i : data) {
            result += i.ToString() + " ";
        }
        return result;
    }

    template <class TCoord>
    static void SerializeVectorToJson(NJson::TJsonWriter& w, const TString& keyName, const TVector<TCoord>& data) {
        w.OpenArray(keyName);
        for (auto&& c : data) {
            w.Write(c.X);
            w.Write(c.Y);
        }
        w.CloseArray();
    }

    template <class TCoord>
    static NJson::TJsonValue SerializeVectorToJson(const TVector<TCoord>& data) {
        NJson::TJsonValue result(NJson::JSON_ARRAY);
        for (auto&& i : data) {
            result.AppendValue(i.X);
            result.AppendValue(i.Y);
        }
        return result;
    }

    template <class TCoord>
    static NJson::TJsonValue SerializeVectorToJsonIFace(const TVector<TCoord>& data) {
        NJson::TJsonValue result(NJson::JSON_ARRAY);
        for (auto&& i : data) {
            NJson::TJsonValue coord;
            coord.AppendValue(i.X);
            coord.AppendValue(i.Y);
            result.AppendValue(coord);
        }
        return result;
    }

    template <class TCoord>
    static TCoord CalcCenter(const TVector<TCoord>& data) {
        TCoord result(0, 0);
        if (!data.empty()) {
            for (auto&& i : data) {
                result = result + i;
            }
            result = result / data.size();
        }
        return result;
    }

    template <class TCoord>
    Y_WARN_UNUSED_RESULT static bool DeserializeVectorFromJsonIFace(const NJson::TJsonValue& data, TVector<TCoord>& result) {
        result.clear();
        if (!data.IsArray()) {
            return false;
        }
        for (auto&& i : data.GetArraySafe()) {
            if (!i.IsArray() || i.GetArraySafe().size() != 2) {
                return false;
            }

            if (!i.GetArraySafe()[0].IsDouble() || !i.GetArraySafe()[1].IsDouble()) {
                return false;
            }

            TCoord coord;
            coord.X = i.GetArraySafe()[0].GetDouble();
            coord.Y = i.GetArraySafe()[1].GetDouble();
            result.emplace_back(std::move(coord));
        }
        return true;
    }

    template <class TIt>
    static TString SerializeVector(TIt itBegin, TIt itEnd) {
        TString result;
        for (auto i = itBegin; i != itEnd; ++i) {
            result += i->ToString() + " ";
        }
        return result;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromStringLonLat(const TStringBuf& dataExt, const char delim = ' ') {
        TStringBuf data = dataExt;
        while (data.StartsWith(" ")) {
            data.Skip(1);
        }
        while (data.EndsWith(" ")) {
            data.Chop(1);
        }
        TStringBuf l;
        TStringBuf r;
        if (!data.TrySplit(delim, l, r)) {
            return false;
        }
        return TryFromString<T>(l, X) && TryFromString<T>(r, Y);
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromString(const TStringBuf& dataExt, const char delim = ' ') {
        return DeserializeFromStringLonLat(dataExt, delim);
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromStringLatLon(const TStringBuf& dataExt, const char delim = ' ') {
        TStringBuf data = dataExt;
        while (data.StartsWith(" ")) {
            data.Skip(1);
        }
        while (data.EndsWith(" ")) {
            data.Chop(1);
        }
        TStringBuf l;
        TStringBuf r;
        if (!data.TrySplit(delim, l, r)) {
            return false;
        }
        return TryFromString<T>(l, Y) && TryFromString<T>(r, X);
    }

    TBaseCoord(const TStringBuf& data) {
        TStringBuf buf(data);
        ui32 pos = 0;
        while (pos < buf.length() && buf[pos] == ' ') {
            ++pos;
        }

        ui32 pos1 = pos;
        while (pos1 < buf.length() && buf[pos1] != ' ') {
            ++pos1;
        }
        Y_ENSURE(pos1 > pos && pos < buf.length());
        X = FromString<T>(buf.SubStr(pos, pos1 - pos));

        pos = pos1;
        while (pos < buf.length() && buf[pos] == ' ') {
            ++pos;
        }

        pos1 = pos;
        while (pos1 < buf.length() && buf[pos1] != ' ') {
            ++pos1;
        }
        Y_ENSURE(pos1 > pos && pos < buf.length());
        Y = FromString<T>(buf.SubStr(pos, pos1 - pos));
    }

    Y_FORCE_INLINE bool Compare(const TSelf& c, double delta) const {
        return Abs(X - c.X) < delta && Abs(Y - c.Y) < delta;
    }

    TSelf operator+ (const TSelf& c) const {
        return TSelf(X + c.X, Y + c.Y);
    }

    TSelf operator- (const TSelf& c) const {
        return TSelf(X - c.X, Y - c.Y);
    }

    TSelf operator* (const TCoordType& c) const {
        return TSelf(X * c, Y * c);
    }

    TSelf operator/ (const TCoordType& kf) const {
        return TSelf(X / kf, Y / kf);
    }

    template <class TProto>
    Y_WARN_UNUSED_RESULT bool Deserialize(const TProto& coord) {
        if (coord.HasXHP()) {
            X = coord.GetXHP();
            Y = coord.GetYHP();
        } else {
            X = coord.GetX();
            Y = coord.GetY();
        }
        return true;
    }

    NRTYGeometry::TSimpleCoord Serialize() const {
        NRTYGeometry::TSimpleCoord result;
        result.SetX(X);
        result.SetY(Y);
        result.SetXHP(X);
        result.SetYHP(Y);
        return result;
    }

    NRTYGeometry::TSimpleCoord SerializeHP() const {
        NRTYGeometry::TSimpleCoord result;
        result.SetXHP(X);
        result.SetYHP(Y);
        return result;
    }

    NRTYGeometry::TSimpleCoord SerializeLP() const {
        NRTYGeometry::TSimpleCoord result;
        result.SetX(X);
        result.SetY(Y);
        return result;
    }

    Y_FORCE_INLINE double SimpleLength() const {
        return sqrt(X * X + Y * Y);
    }

    Y_FORCE_INLINE TString ToStringLonLat() const {
        return ::ToString(X) + " " + ::ToString(Y);
    }

    Y_FORCE_INLINE TString ToStringLatLon() const {
        return ::ToString(Y) + " " + ::ToString(X);
    }

    Y_FORCE_INLINE TString ToString() const {
        return ToStringLonLat();
    }

    Y_FORCE_INLINE NJson::TJsonValue SerializeToJson() const {
        NJson::TJsonValue jsonCoord;
        jsonCoord.AppendValue(X);
        jsonCoord.AppendValue(Y);
        return jsonCoord;
    }

    Y_WARN_UNUSED_RESULT bool DeserializeFromJson(const NJson::TJsonValue& jsonCoord) {
        if (jsonCoord.IsArray()) {
            if (jsonCoord.GetArray().size() != 2 || !jsonCoord.GetArray()[0].IsDouble()) {
                TFLEventLog::Log("coord_parsing:incorrect_array_types")("raw_data", jsonCoord.GetStringRobust());
                return false;
            }
            X = jsonCoord.GetArray()[0].GetDouble();
            Y = jsonCoord.GetArray()[1].GetDouble();
            return true;
        } else if (jsonCoord.IsString()) {
            return DeserializeFromStringLonLat(jsonCoord.GetString());
        }
        TFLEventLog::Log("coord_parsing:incorrect_json_type")("raw_data", jsonCoord.GetStringRobust());
        return false;
    }

};

template <class T, class TProductExternal>
struct TEuclideCoord: public TBaseCoord<T, TProductExternal> {
    using TProduct = TProductExternal;
    using TBase = TBaseCoord<T, TProductExternal>;
    using TSelf = TProductExternal;
    using TCoordType = typename TBase::TCoordType;
    using TBase::X;
    using TBase::Y;

    static double GetArea(const TVector<TSelf>& coords) {
        if (coords.size() <= 2) {
            return 0;
        }
        double result = 0;
        for (ui32 i = 0; i + 1 < coords.size(); ++i) {
            result += (coords[i + 1].Y + coords[i].Y) * (coords[i + 1].X - coords[i].X);
        }
        result += (coords.back().Y + coords.front().Y) * (coords.front().X - coords.back().X);
        result *= 0.5;
        return Abs(result);
    }

    Y_FORCE_INLINE double GetLengthTo(const TBase& c) const {
        double dx = c.X - X;
        double dy = c.Y - Y;
        return sqrt(dx * dx + dy * dy);
    }

    Y_FORCE_INLINE T MakeDXFromDistance(const double delta) const {
        return delta;
    }

    Y_FORCE_INLINE T MakeDYFromDistance(const double delta) const {
        return delta;
    }

    using TBase::TBase;

    template <bool Normalized = false>
    Y_FORCE_INLINE void GetProducts(const TSelf& to1, const TSelf& to2, T& scalar, T& vector) const {
        vector = (to1.X - X) * (to2.Y - Y) - (to1.Y - Y) * (to2.X - X);
        scalar = (to1.X - X) * (to2.X - X) + (to1.Y - Y) * (to2.Y - Y);
    }

    Y_FORCE_INLINE T GetVectorProduct(const TSelf& to1, const TSelf& to2) const {
        return (to1.X - X) * (to2.Y - Y) - (to1.Y - Y) * (to2.X - X);
    }

    Y_FORCE_INLINE T GetScalarProduct(const TSelf& to1, const TSelf& to2) const {
        return (to1.X - X) * (to2.X - X) + (to1.Y - Y) * (to2.Y - Y);
    }

    TSelf ProjectTo(const TSelf& c1, const TSelf& c2, double* length = nullptr, double* alphaResult = nullptr) const {
        double vx = c2.X - c1.X;
        double vy = c2.Y - c1.Y;

        double vl = vx * vx + vy * vy;

        if (vl < 1e-15) {
            if (length) {
                *length = GetLengthTo(c1);
            }
            if (alphaResult) {
                *alphaResult = 0.5;
            }

            return c1;
        }

        double rx = X - c1.X;
        double ry = Y - c1.Y;

        double alpha = (rx * vx + ry * vy) / vl;

        if (alphaResult) {
            *alphaResult = alpha;
        }

        if (alpha <= 0) {
            if (length) {
                *length = GetLengthTo(c1);
            }
            return c1;
        }
        if (alpha >= 1) {
            if (length) {
                *length = GetLengthTo(c2);
            }
            return c2;
        }

        if (length) {
            *length = Abs(rx * vy - vx * ry) / sqrt(vl);
        }

        return c1 * (1 - alpha) + c2 * alpha;
    }

    Y_FORCE_INLINE double GetLengthTo(const TSelf& c1, const TSelf& c2) const {
        double vx = c2.X - c1.X;
        double vy = c2.Y - c1.Y;

        double vl = vx * vx + vy * vy;

        if (vl < 1e-5) {
            return GetLengthTo(c1);
        }

        double rx = TSelf::X - c1.X;
        double ry = TSelf::Y - c1.Y;

        double alpha = (rx * vx + ry * vy) / vl;

        if (alpha <= 0)
            return GetLengthTo(c1);
        if (alpha >= 1)
            return GetLengthTo(c2);

        return Abs(rx * vy - vx * ry) / sqrt(vl);
    }

    static bool Cross(const TSelf& c00, const TSelf& c01, const TSelf& c10, const TSelf& c11, TSelf* result = nullptr) {
        double v0x = c01.X - c00.X;
        double v0y = c01.Y - c00.Y;

        double v1x = c11.X - c10.X;
        double v1y = c11.Y - c10.Y;

        double rx = c10.X - c00.X;
        double ry = c10.Y - c00.Y;

        double d = v0x * v1y - v1x * v0y;
        double d0 = rx * v0y - ry * v0x;
        double d1 = rx * v1y - ry * v1x;

        if (Abs(d) < 1e-10)
            return false;

        double t0 = d1 / d;
        double t1 = d0 / d;

        if (t0 > 1 || t0 < 0 || t1 > 1 || t1 < 0)
            return false;

        if (result) {
            *result = c00 + (c01 - c00) * t0;
            auto c1 = c00 + (c01 - c00) * t0;
            auto c2 = c10 + (c11 - c10) * t1;
            Y_ASSERT(c1.GetLengthTo(c2) < 1e-10);
        }

        return true;
    }

    Y_FORCE_INLINE bool BuildMoved(const TSelf& v, const double length, TSelf& result) {
        const double l = v.SimpleLength();
        if (l < 1e-5)
            return false;
        result = *this + v * length / l;
        return true;
    }

    static bool BuildNormal(const TSelf& c1, const TSelf& c2, const double length, TSelf& result) {
        double d = c1.GetLengthTo(c2);
        if (d < 1e-5 || Abs(length) < 1e-5)
            return false;
        TCoordType dx = c2.X - c1.X;
        TCoordType dy = c2.Y - c1.Y;

        result.X = -dy / d * length;
        result.Y = dx / d * length;

        Y_ASSERT(Abs(c1.GetLengthTo(result + c1) / length - 1) < 1e-5);

        return true;
    }

    static TCoordType CalcRectArea(const TSelf& min, const TSelf& max) {
        return (max.X - min.X) * (max.Y - min.Y);
    }

    ui32 CrossSphere(const TCoordType r, const TSelf& c1, const TSelf& c2, TSelf& r1, TSelf& r2, const double lengthPrecision) const {
        TCoordType sx = c2.X - c1.X;
        TCoordType sy = c2.Y - c1.Y;

        TCoordType dx = c1.X - X;
        TCoordType dy = c1.Y - Y;

        double a = sx * sx + sy * sy;

        if (a < 1e-10) {
            if (Abs(GetLengthTo(c1) - r) < 1e-5) {
                r1 = c1;
                r2 = c2;
                return 2;
            }
            return 0;
        }

        double b = 2 * (sx * dx + sy * dy);
        double c = dx * dx + dy * dy - r * r;

        double DS = b * b - 4 * a * c;
        if (DS < 0) {
            return 0;
        }

        double t0, t1;
        if (b > 0) {
            t0 = (-b - sqrt(DS)) / (2 * a);
            t1 = c / t0;
        } else {
            t1 = (-b + sqrt(DS)) / (2 * a);
            t0 = c / t1;
        }

        r1 = c1 + (c2 - c1) * t0;
        r2 = c1 + (c2 - c1) * t1;

        double delta = lengthPrecision / c2. GetLengthTo(c1);

        return ((t0 >= -delta && t0 <= 1 + delta) ? 1 : 0) + ((t1 >= -delta && t1 <= 1 + delta) ? 1 : 0);
    }

    static double GetSin(const TSelf& p1, const TSelf& c, const TSelf& p2) {
        const double l1 = c.GetLengthTo(p1);
        const double l2 = c.GetLengthTo(p2);
        if (l1 < 1e-5 || l2 < 1e-5)
            return 0;
        return c.GetVectorProduct(p1, p2) / (l1 * l2);
    }

    static double GetCos(const TSelf& p1, const TSelf& c, const TSelf& p2) {
        const double l1 = c.GetLengthTo(p1);
        const double l2 = c.GetLengthTo(p2);
        if (l1 < 1e-5 || l2 < 1e-5)
            return 1;
        return c.GetScalarProduct(p1, p2) / (l1 * l2);
    }
};

template <class TProductExternal>
struct TSphereCoord: public TBaseCoord<double, TProductExternal> {
private:
    using TSelf = TProductExternal;
    using TBase = TBaseCoord<double, TProductExternal>;
    using TEuclideCoord = TEuclideCoord<double, TProductExternal>;
public:
    using TProduct = TProductExternal;
    using TBase::X;
    using TBase::Y;

    using TCoordType = typename TBase::TCoordType;
    static const double EarthR;
    using TBase::TBase;

   Y_WARN_UNUSED_RESULT bool DeserializeLatLonFromJson(const NJson::TJsonValue& json) {
        JREAD_DOUBLE(json, "lon", X);
        JREAD_DOUBLE(json, "lat", Y);
        return true;
    }

    NJson::TJsonValue SerializeLatLonToJson() const {
        NJson::TJsonValue result;
        result.InsertValue("lon", X);
        result.InsertValue("lat", Y);
        return result;
    }

    Y_WARN_UNUSED_RESULT static bool DeserializeLatLonVectorFromJson(const NJson::TJsonValue& json, TVector<TProductExternal>& result) {
        const NJson::TJsonValue::TArray* arr;
        if (!json.GetArrayPointer(&arr)) {
            return false;
        }
        for (auto&& i : *arr) {
            TProductExternal p;
            if (!p.DeserializeLatLonFromJson(i)) {
                return false;
            }
            result.emplace_back(std::move(p));
        }
        return true;
    }

    Y_FORCE_INLINE double GetSigma() const {
        return cos(Y / 180.0 * M_PI);
    }

    static double GetArea(const TVector<TSelf>& coords) {
        if (coords.size() <= 2) {
            return 0;
        }
        double result = 0;
        double kf = EarthR * M_PI / 180;
        for (ui32 i = 0; i + 1 < coords.size(); ++i) {
            result += coords[i].GetSigma() * (coords[i + 1].X + coords[i].X) * (coords[i + 1].Y - coords[i].Y);
        }
        result += coords.front().GetSigma() * (coords.back().X + coords.front().X) * (coords.front().Y - coords.back().Y);
        result *= kf * kf * 0.5;
        return Abs(result);
    }

    template <bool Normalized = false>
    void GetProducts(const TSelf& to1, const TSelf& to2, double& scalar, double& vector) const {
        const double sigma = GetSigma();

        constexpr double kf = Normalized ? 1 : (EarthR * M_PI / 180);

        const double dx1 = (to1.X - X) * kf;
        const double dx2 = (to2.X - X) * kf;
        const double dy1 = (to1.Y - Y) * sigma * kf;
        const double dy2 = (to2.Y - Y) * sigma * kf;
        scalar = dx1 * dx2 + dy1 * dy2;
        vector = dx1 * dy2 - dy1 * dx2;
    }

    template <bool Normalized = false>
    Y_FORCE_INLINE double GetVectorProduct(const TSelf& to1, const TSelf& to2) const {
        const double kff = (Normalized ? 1 : (EarthR * M_PI / 180));
        return ((to1.X - X) * (to2.Y - Y) - (to1.Y - Y) * (to2.X - X)) * GetSigma() * kff * kff;
    }

    template <bool Normalized = false>
    Y_FORCE_INLINE double GetScalarProduct(const TSelf& to1, const TSelf& to2) const {
        const double sigma = GetSigma();
        const double kff = (Normalized ? 1 : (EarthR * M_PI / 180));
        return ((to1.X - X) * (to2.X - X) + (to1.Y - Y) * (to2.Y - Y) * sigma * sigma) * kff * kff;
    }

    static bool BuildNormal(const TSelf& c1, const TSelf& c2, const double length, TSelf& result) {
        double d = c1.GetLengthTo(c2);
        if (d < 1e-5 || Abs(length) < 1e-5) {
            return false;
        }
        double sigma = cos(c1.Y / 180.0 * M_PI);
        TCoordType dx = c2.X - c1.X;
        TCoordType dy = c2.Y - c1.Y;

        result.X = -dy / (d * sigma) * length;
        result.Y = dx / d * (sigma * length);

        Y_ASSERT(Abs(c1.GetLengthTo(c1 + result) - Abs(length)) < 1e-2);

        return true;
    }

    void NormalizeCoord() {
        if (X >= 180) {
            X = X - 360 * int((X + 180) / 360);
        }
        if (X < -180) {
            X = - (-X - 360 * int((-X + 180) / 360));
        }
        if (Y >= 180) {
            Y = Y - 360 * int((Y + 180) / 360);
        }
        if (Y < -180) {
            Y = -(-Y - 360 * int((-Y + 180) / 360));
        }
        if (Y >= 90) {
            Y = 180 - Y;
        }
        if (Y < -90) {
            Y = - (180 + Y);
        }
    }

    template <class TProto>
    bool Deserialize(const TProto& coord) {
        TBase::Deserialize(coord);
        NormalizeCoord();
        return true;
    }

    Y_FORCE_INLINE bool BuildMoved(const TSelf& v, const double length, TSelf& result) {
        const double l = GetLengthTo(*this + v);
        if (l < 1e-5)
            return false;
        result = *this + v * length / l;
        return true;
    }

    Y_FORCE_INLINE double MakeDXFromDistance(const double delta) const {
        if (Abs(Abs(Y) - 90) < 1e-5) {
            return 0;
        }
        return delta / (EarthR * cos(Y / 180.0 * M_PI)) * 180 / M_PI;
    }

    Y_FORCE_INLINE double MakeDYFromDistance(const double delta) const {
        return delta / EarthR * 180 / M_PI;
    }

    static TCoordType CalcRectArea(const TSelf& min, const TSelf& max) {
        double minY = Abs(min.Y);
        double maxY = Abs(max.Y);

        int segmMin = (int)(minY) / 90;
        int segmMax = (int)(maxY) / 90;

        double lMax = segmMax;
        double lMin = segmMin;

        if (segmMin % 2 == 0) {
            lMin += Abs(sin(minY / 180 * M_PI));
        } else {
            lMin += 1 - Abs(sin(minY / 180 * M_PI));
        }

        if (segmMax % 2 == 0) {
            lMax += Abs(sin(maxY / 180 * M_PI));
        } else {
            lMax += 1 - Abs(sin(maxY / 180 * M_PI));
        }

        if (max.Y < 0)
            lMax *= -1;

        if (min.Y < 0)
            lMin *= -1;

        return (max.X - min.X) / 180 * M_PI * EarthR * EarthR * (lMax - lMin);
    }

    template <bool Normalized = false>
    Y_FORCE_INLINE double GetLengthTo(const TBase& c) const {
//        Y_ASSERT(Abs(Y) <= 180);
//        Y_ASSERT(Abs(X) <= 180);

//        Y_ASSERT(Abs(c.Y) <= 180);
//        Y_ASSERT(Abs(c.X) <= 180);

        double xRad1 = Y;
        double xRad2 = c.Y;

        double approxS1 = xRad2 - xRad1;
        double approxS2 = (c.X - X);
        double cc = cos(xRad1 * M_PI / 180.0) * cos(xRad2 * M_PI / 180.0);
        return (Normalized ? 1 : (EarthR * M_PI / 180.0)) * (sqrt(approxS1 * approxS1 + cc * approxS2 * approxS2));

//        double s1 = sin((xRad2 - xRad1) / 2);
//        double s2 = sin((yRad2 - yRad1) / 2);
//        double cc = cos(xRad1) * cos(xRad2);
//        return 2 * r * asin(sqrt(s1 * s1 + cc * s2 * s2));
    }

    ui32 CrossSphere(const TCoordType r0, const TSelf& c1, const TSelf& c2, TSelf& r1, TSelf& r2, const double lengthPrecision) const {
        double l;
        TSelf cProj = ProjectTo(c1, c2, &l);
        if (l > r0) {
            return 0;
        }

        double r = r0 * (180.0 / M_PI);
        TCoordType sx = EarthR * (c2.X - c1.X);
        TCoordType sy = EarthR * (c2.Y - c1.Y);

        TCoordType dx = EarthR * (c1.X - X);
        TCoordType dy = EarthR * (c1.Y - Y);

        double alpha = cos(cProj.Y / 180.0 * M_PI) * cos(Y / 180.0 * M_PI);

        double a = alpha * sx * sx + sy * sy;

        if (a < 1e-10) {
            if (Abs(GetLengthTo(c1) - r0) < 1e-5) {
                r1 = c1;
                r2 = c2;
                return 2;
            }
            return 0;
        }

        double b = 2 * (alpha * sx * dx + sy * dy) / a;
        double c = (alpha * dx * dx + dy * dy - r * r) / a;

        double DS = b * b - 4 * c;
        if (DS < 0) {
            return 0;
        }

        double t0, t1;
        if (b > 0) {
            t0 = 0.5 * (-b - sqrt(DS));
            t1 = c / t0;
        } else {
            t1 = 0.5 * (-b + sqrt(DS));
            t0 = c / t1;
        }

        r1 = c1 + (c2 - c1) * t0;
        r2 = c1 + (c2 - c1) * t1;

        Y_ASSERT(Abs(GetLengthTo(r1) / r0 - 1) < 1e-2);
        Y_ASSERT(Abs(GetLengthTo(r2) / r0 - 1) < 1e-2);

        double delta = lengthPrecision / c1.GetLengthTo(c2);

        return ((t0 >= -delta && t0 <= 1 + delta) ? 1 : 0) + ((t1 >= -delta && t1 <= 1 + delta) ? 1 : 0);
    }

    Y_FORCE_INLINE TSelf ProjectTo(const TSelf& c1, const TSelf& c2, double* length = nullptr, double* alphaResult = nullptr) const {
        double sigma = cos(Y / 180.0 * M_PI);
        double vx = (c2.X - c1.X) * sigma;
        double vy = c2.Y - c1.Y;

        double vl = vx * vx + vy * vy;

        TSelf result;
        double alpha = 0.5;

        if (vl < 1e-15) {
            result = c1;
        } else {
            double rx = (X - c1.X) * sigma;
            double ry = Y - c1.Y;

            alpha = (rx * vx + ry * vy) / vl;

            if (alpha <= 0) {
                result = c1;
            } else if (alpha >= 1) {
                result = c2;
            } else {
                result = c1 * (1 - alpha) + c2 * alpha;
            }
        }

        if (alphaResult) {
            *alphaResult = alpha;
        }

        if (length) {
            *length = result.GetLengthTo(*this);
        }

        return result;
    }

    static bool Cross(const TSelf& c00, const TSelf& c01, const TSelf& c10, const TSelf& c11, TSelf* result = nullptr) {
        double v0x = c01.X - c00.X;
        double v0y = c01.Y - c00.Y;

        double v1x = c11.X - c10.X;
        double v1y = c11.Y - c10.Y;

        double rx = c10.X - c00.X;
        double ry = c10.Y - c00.Y;

        double d = v0x * v1y - v1x * v0y;
        double d0 = rx * v0y - ry * v0x;
        double d1 = rx * v1y - ry * v1x;

        if (Abs(d) < 1e-15)
            return false;

        double t0 = d1 / d;
        double t1 = d0 / d;

        if (t0 > 1 || t0 < 0 || t1 > 1 || t1 < 0)
            return false;

        if (result) {
            *result = c00 + (c01 - c00) * t0;
        }
        Y_ASSERT((c00 + (c01 - c00) * t0).GetLengthTo(c10 + (c11 - c10) * t1) < 1);

        return true;
    }

    Y_FORCE_INLINE double GetLengthTo(const TSelf& c1, const TSelf& c2) const {
        double length;
        ProjectTo(c1, c2, &length);
        return length;
    }

    static double GetSin(const TSelf& p1, const TSelf& c, const TSelf& p2) {
        TSelf v1Geo(p1.X - c.X, p1.Y - c.Y);
        TSelf v2Geo(p2.X - c.X, p2.Y - c.Y);

        TEuclideCoord v1Eucl = TEuclideCoord(v1Geo.X, c.GetSigma() * v1Geo.Y);
        TEuclideCoord v2Eucl = TEuclideCoord(v2Geo.X, c.GetSigma() * v2Geo.Y);

        double l1 = v1Eucl.SimpleLength();
        double l2 = v2Eucl.SimpleLength();
        if (l1 < 1e-5 || l2 < 1e-5) {
            return 0;
        }

        double vectorProd = v1Eucl.X * v2Eucl.Y - v1Eucl.Y * v2Eucl.X;

        return vectorProd / (l1 * l2);
    }

    static double GetCos(const TSelf& p1, const TSelf& c, const TSelf& p2) {
        TSelf v1Geo(p1.X - c.X, p1.Y - c.Y);
        TSelf v2Geo(p2.X - c.X, p2.Y - c.Y);

        TEuclideCoord v1Eucl = TEuclideCoord(v1Geo.X, c.GetSigma() * v1Geo.Y);
        TEuclideCoord v2Eucl = TEuclideCoord(v2Geo.X, c.GetSigma() * v2Geo.Y);

        double l1 = v1Eucl.SimpleLength();
        double l2 = v2Eucl.SimpleLength();
        if (l1 < 1e-5 || l2 < 1e-5) {
            return 1;
        }

        double scalarProd = v1Eucl.X * v2Eucl.X + v1Eucl.Y * v2Eucl.Y;
        return scalarProd / (l1 * l2);
    }

    static void FilterCoords(const TVector<TSelf>& allPoints, TVector<TSelf>& out, double coeff) {
        Y_ASSERT(allPoints.size() > 1);
        out.push_back(allPoints.front());

        for (ui32 i = 2; i < allPoints.size(); ++i) {
            double sin = TSelf::GetSin(out.back(), allPoints[i - 1], allPoints[i]);
            if (Abs(sin) < coeff) {
                continue;
            }
            out.push_back(allPoints[i - 1]);
        }
        out.push_back(allPoints.back());
    }
};

template <class TProductExternal>
const double TSphereCoord<TProductExternal>::EarthR = 6372795;

template <class T>
class TTraceCoordTimestamp: public T {
private:
    using TSelf = TTraceCoordTimestamp;
    using TBase = T;
protected:
    ui32 Timestamp = 0;
public:

    using TBaseCoord = T;
    TTraceCoordTimestamp() = default;

    using TBase::X;
    using TBase::Y;

    TTraceCoordTimestamp(ui32 timestamp, double x, double y)
        : TBase(x, y) {
        Timestamp = timestamp;
    }

    TTraceCoordTimestamp(ui32 timestamp, const T& c)
        : TBase(c) {
        Timestamp = timestamp;
    }

    Y_FORCE_INLINE TSelf& SetTimestamp(const ui32 timestamp) {
        Timestamp = timestamp;
        return *this;
    }

    Y_FORCE_INLINE ui32 GetTimestamp() const {
        return Timestamp;
    }

    void SerializeToProto(::NRTYGeometry::TCoord& protoCoord) const {
        protoCoord.SetX(X);
        protoCoord.SetY(Y);
        protoCoord.SetTimestamp(Timestamp);
    }

    bool DeserializeFromProto(const ::NRTYGeometry::TCoord& protoCoord) {
        X = protoCoord.GetX();
        Y = protoCoord.GetY();
        Timestamp = protoCoord.GetTimestamp();
        return true;
    }

    TInstant GetInstant() const {
        return TInstant::Seconds(Timestamp);
    }

    TString ToString() const {
        return TBase::ToString() + " " + ::ToString(Timestamp);
    }

    bool EqualByTime(const TSelf& c) const {
        return Timestamp == c.Timestamp;
    }

    bool operator <(const TSelf& item) const {
        return Timestamp < item.Timestamp;
    }

};

template <class T>
class TTraceCoordSpeedImpl : public TTraceCoordTimestamp<T> {
private:
    using TSelf = TTraceCoordSpeedImpl<T>;
    using TBase = TTraceCoordTimestamp<T>;
protected:
    float Speed = 0;
    using TBase::Timestamp;
public:

    using TBaseCoord = T;
    using TBase::TBase;
    TTraceCoordSpeedImpl() = default;

    using TBase::X;
    using TBase::Y;

    TSelf& SetSpeed(const float speed) {
        Speed = speed;
        return *this;
    }

    float GetSpeed() const {
        return Speed;
    }

    void SerializeToProto(::NRTYGeometry::TCoord& protoCoord) const {
        TBase::SerializeToProto(protoCoord);
        protoCoord.SetSpeed(Speed);
    }

    bool DeserializeFromProto(const ::NRTYGeometry::TCoord& protoCoord) {
        if (!TBase::DeserializeFromProto(protoCoord)) {
            return false;
        }
        Speed = protoCoord.GetSpeed();
        return true;
    }

};

template <class T>
class TTraceCoordFull: public TTraceCoordSpeedImpl<T> {
private:
    using TSelf = TTraceCoordFull;
    using TBase = TTraceCoordSpeedImpl<T>;
    ui32 Marker = 0;
    ui16 Ms = 0;
    using TBase::Timestamp;
public:
    using TBaseCoord = T;
    using TBase::TBase;

    TTraceCoordFull() = default;

    Y_FORCE_INLINE TSelf& SetMs(const ui16 ms) {
        Ms = ms;
        return *this;
    }

    Y_FORCE_INLINE ui16 GetMs() const {
        return Ms;
    }

    Y_FORCE_INLINE TSelf& SetMarker(const ui32 marker) {
        Marker = marker;
        return *this;
    }

    Y_FORCE_INLINE ui32 GetMarker() const {
        return Marker;
    }

    bool DeserializeFromProto(const ::NRTYGeometry::TCoord& protoCoord) {
        if (!TBase::DeserializeFromProto(protoCoord)) {
            return false;
        }
        Ms = protoCoord.GetMs();
        Marker = protoCoord.GetMarker();
        return true;
    }

    void SerializeToProto(::NRTYGeometry::TCoord& protoCoord) const {
        TBase::SerializeToProto(protoCoord);
        protoCoord.SetMs(Ms);
        protoCoord.SetMarker(Marker);
    }

    TInstant GetInstant() const {
        return TInstant::MilliSeconds((ui64)Timestamp * 1000 + Ms);
    }

    bool EqualByTime(const TSelf& c) const {
        return (Timestamp == c.Timestamp) && (Ms == c.Ms);
    }

    bool operator <(const TSelf& item) const {
        if (Timestamp == item.Timestamp)
            return Ms < item.Ms;
        return Timestamp < item.Timestamp;
    }

};

template <class T>
class TTraceCoordImpl: public T {
private:
    using TSelf = TTraceCoordImpl<T>;
    using TBase = T;

public:

    Y_FORCE_INLINE const TSelf& OriginalCoord() const {
        return *this;
    }

    using TBase::GetInstant;
    using TBase::GetTimestamp;

    bool DeserializeFromProto(const ::NRTYGeometry::TCoord& protoCoord) {
        return T::DeserializeFromProto(protoCoord);
    }

    void SerializeToProto(::NRTYGeometry::TCoord& protoCoord) const {
        return T::SerializeToProto(protoCoord);
    }

    using TCoordType = typename TBase::TCoordType;
    using TBase::X;
    using TBase::Y;
    using TBase::TBase;
    TTraceCoordImpl() {
    }
/*
    Y_WARN_UNUSED_RESULT static bool DeserializeVector(const TString& data, TVector<TSelf>& result) {
        result.clear();
        TVector<TString> coords = SplitString(data, " ");
        if (coords.size() % 3)
            return false;
        try {
            for (ui32 i = 0; i < coords.size() / 3; ++i) {
                result.push_back(TSelf(FromString<TBase::TCoordType>(coords[3 * i]), FromString<TBase::TCoordType>(coords[3 * i + 1]), FromString<ui32>(coords[3 * i + 2])));
            }
        } catch (...) {
            return false;
        }
        return true;
    }

    static TString SerializeVector(const TVector<TSelf>& data) {
        TString result;
        for (auto&& i : data) {
            result += i.ToString() + " ";
        }
        return result;
    }
*/
    static void Split(const TVector<TSelf>& data, TVector<typename T::TBaseCoord>& coords, TVector<ui64>& timestamps) {
        coords.resize(data.size());
        timestamps.resize(data.size());
        for (ui32 i = 0; i < data.size(); ++i) {
            coords[i] = data[i];
            timestamps[i] = data[i].GetTimestamp();
        }
    }

    static bool Join(const TVector<typename T::TBaseCoord>& coords, const TVector<ui64>& timestamps, TVector<TSelf>& result) {
        if (coords.size() != timestamps.size())
            return false;
        result.resize(coords.size());
        for (ui32 i = 0; i < result.size(); ++i) {
            result[i] = TSelf(timestamps[i], coords[i].X, coords[i].Y);
        }
        return true;
    }

};

template <class TCoordPolicy>
class TCommonCoord: public TCoordPolicy {
private:
    using TBase = TCoordPolicy;
    using TSelf = typename TCoordPolicy::TProduct;
public:

    using TCoordType = typename TBase::TCoordType;

    using TBase::X;
    using TBase::Y;

    TCommonCoord()
        : TBase()
    {
    }

    TCommonCoord(const TCoordType& x, const TCoordType& y)
        : TBase(x, y)
    {
    }

    TCommonCoord(const char* data)
        : TBase(TString(data))
    {
    }

    TCommonCoord(const TStringBuf& data)
        : TBase(data)
    {
    }

    TCommonCoord(const TString& data)
        : TBase(data)
    {
    }

    bool IsEmpty() const {
        return Abs(X) < 1e-5 && Abs(Y) < 1e-5;
    }

    ui64 GetHash() const {
        return MultiHash(X, Y);
    }

    Y_FORCE_INLINE EMutualPosition GetMutualPosition(const TSelf& baseDirection, const TSelf& interestDirection) const {
        double vProd = TBase::GetVectorProduct(baseDirection, interestDirection);
        if (vProd > 1e-5) {
            return EMutualPosition::mpLeft;
        }
        if (vProd < -1e-5) {
            return EMutualPosition::mpRight;
        }
        double sProd = TBase::GetScalarProduct(baseDirection, interestDirection);
        if (sProd > 1e-5) {
            return EMutualPosition::mpSame;
        }
        if (sProd < -1e-5) {
            return EMutualPosition::mpReverse;
        }
        return EMutualPosition::mpCantDetect;
    }

};

template <class T>
class TCoord: public TCommonCoord<TEuclideCoord<T, TCoord<T>>> {
private:
    using TBase = TCommonCoord<TEuclideCoord<T, TCoord<T>>>;
public:
    using TBase::TBase;
};

class TGeoCoord: public TCommonCoord<TSphereCoord<TGeoCoord>> {
private:
    using TBase = TCommonCoord<TSphereCoord<TGeoCoord>>;
public:
    using TBase::TBase;

    double& MutableLatitude() {
        return Y;
    }

    double& MutableLongitude() {
        return X;
    }

    TGeoCoord& SetLatitude(const double lat) {
        Y = lat;
        return *this;
    }

    TGeoCoord& SetLongitude(const double lon) {
        X = lon;
        return *this;
    }

    double GetLatitude() const {
        return Y;
    }

    double GetLongitude() const {
        return X;
    }

    bool operator<(const TGeoCoord& other) const {
        return X < other.X || (X == other.X && Y < other.Y);
    }

    bool DeserializeFromJson(const NJson::TJsonValue& jsonCoord) {
        if (jsonCoord.IsMap()) {
            return DeserializeLatLonFromJson(jsonCoord);
        } else {
            return TBase::DeserializeFromJson(jsonCoord);
        }
    }
};

using TSimpleCoord = TCoord<double>;
using TTraceCoord = TTraceCoordImpl<TTraceCoordFull<TGeoCoord>>;
using TTraceCoordSimple = TTraceCoordImpl<TTraceCoordTimestamp<TGeoCoord>>;
using TTraceCoordSpeed = TTraceCoordImpl<TTraceCoordSpeedImpl<TGeoCoord>>;
