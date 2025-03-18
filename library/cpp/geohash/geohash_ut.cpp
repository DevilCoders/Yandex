#include "geohash.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/xrange.h>

using namespace NGeoHash;

static TVector<TString> GetAllPrefixes(const TString& str) {
    TVector<TString> result;
    for (auto i : xrange(str.Size() + 1)) {
        result.push_back(str.substr(0, i));
    }
    return result;
};

static TVector<ui64> GetAllPrefixes(ui64 x, yssize_t size, ui8 step = 5) {
    TVector<ui64> vec;

    for (auto i : xrange(size + 1)) {
        vec.push_back(x);
        x >>= step;
        Y_UNUSED(i);
    }
    Reverse(vec.begin(), vec.end());
    return vec;
};

static const double mskLat = 55.75;
static const double mskLon = 37.616667;
static const auto mskGeoHashes = GetAllPrefixes("ucftpuzx7c5z");
static const auto mskGeoHashBits = GetAllPrefixes(949654441010900159, 12);

static const double spbLat = 59.95;
static const double spbLon = 30.316667;
static const auto spbGeoHashes = GetAllPrefixes("udtt1cefj9cr");
static const auto spbGeoHashBits = GetAllPrefixes(951166665090442615, 12);

/* Достаточно ли такого набора precision'ов?
 * 8 - "ходовое" значение
 * 7 - нечетное значение
 * 1, 12 - крайние, для выведения краевых эффектов */
static const ui8 precisions[] = {1, 4, 7, 8, 12};
static const auto directions = GetEnumAllValues<NGeoHash::EDirection>();

Y_UNIT_TEST_SUITE(TEncodeTest) {
    Y_UNIT_TEST(TestEncodeLatLon) {
        for (auto precision : precisions) {
            UNIT_ASSERT_EQUAL(Encode(mskLat, mskLon, precision), mskGeoHashBits[precision]);
            UNIT_ASSERT_EQUAL(Encode(spbLat, spbLon, precision), spbGeoHashBits[precision]);
        }
    }

    Y_UNIT_TEST(TestEncodePoint) {
        auto mskPoint = NGeo::TPointLL(mskLon, mskLat);
        auto spbPoint = NGeo::TPointLL(spbLon, spbLat);

        for (auto precision : precisions) {
            UNIT_ASSERT_EQUAL(Encode(mskPoint, precision), mskGeoHashBits[precision]);
            UNIT_ASSERT_EQUAL(Encode(spbPoint, precision), spbGeoHashBits[precision]);
        }
    }
}

Y_UNIT_TEST_SUITE(TEncodeToStringTest) {
    Y_UNIT_TEST(TestEncodeLatLonToString) {
        for (auto precision : precisions) {
            UNIT_ASSERT_STRINGS_EQUAL(EncodeToString(mskLat, mskLon, precision), mskGeoHashes[precision]);
            UNIT_ASSERT_STRINGS_EQUAL(EncodeToString(spbLat, spbLon, precision), spbGeoHashes[precision]);
        }
    }

    Y_UNIT_TEST(TestEncodePointToString) {
        auto mskPoint = NGeo::TPointLL(mskLon, mskLat);
        auto spbPoint = NGeo::TPointLL(spbLon, spbLat);
        for (auto precision : precisions) {
            UNIT_ASSERT_STRINGS_EQUAL(EncodeToString(mskPoint, precision), mskGeoHashes[precision]);
            UNIT_ASSERT_STRINGS_EQUAL(EncodeToString(spbPoint, precision), spbGeoHashes[precision]);
        }
    }
}

Y_UNIT_TEST_SUITE(TDecodeToPointTest) {
    Y_UNIT_TEST(TestDecodeBitsToPoint) {
        NGeo::TPointLL truePoints[] = {
            {22.5, 67.5},
            {37.44140625, 55.810546875},
            {37.6165008545, 55.7494354248},
            {37.6166725159, 55.7500362396},
            {37.6166669838, 55.749999946}};
        NGeo::TPointLL testCenterPoint;

        for (auto i : xrange(Y_ARRAY_SIZE(truePoints))) {
            ui8 precision = precisions[i];
            testCenterPoint = DecodeToPoint(mskGeoHashBits[precision], precision);
            UNIT_ASSERT_DOUBLES_EQUAL(testCenterPoint.Lon(), truePoints[i].Lon(), 1e-10);
            UNIT_ASSERT_DOUBLES_EQUAL(testCenterPoint.Lat(), truePoints[i].Lat(), 1e-10);
        }
    }

    Y_UNIT_TEST(TestDecodeStringToPoint) {
        NGeo::TPointLL truePoints[] = {
            {22.5, 67.5},
            {30.41015625, 60.029296875},
            {30.3160858154, 59.9503326416},
            {30.3166007995, 59.9500751495},
            {30.3166670165, 59.9499999638}};
        NGeo::TPointLL testCenterPoint;

        for (auto i : xrange(Y_ARRAY_SIZE(truePoints))) {
            ui8 precision = precisions[i];
            testCenterPoint = DecodeToPoint(spbGeoHashes[precision]);
            UNIT_ASSERT_DOUBLES_EQUAL(testCenterPoint.Lon(), truePoints[i].Lon(), 1e-10);
            UNIT_ASSERT_DOUBLES_EQUAL(testCenterPoint.Lat(), truePoints[i].Lat(), 1e-10);
        }
    }
}

void AssertDoubleBBoxesEqual(const TBoundingBoxLL& first, const TBoundingBoxLL& second) {
    UNIT_ASSERT_DOUBLES_EQUAL(first.GetMinY(), second.GetMinY(), 1e-10);
    UNIT_ASSERT_DOUBLES_EQUAL(first.GetMaxY(), second.GetMaxY(), 1e-10);
    UNIT_ASSERT_DOUBLES_EQUAL(first.GetMinX(), second.GetMinX(), 1e-10);
    UNIT_ASSERT_DOUBLES_EQUAL(first.GetMaxX(), second.GetMaxX(), 1e-10);
}

Y_UNIT_TEST_SUITE(TDecodeToBBoxTest) {
    Y_UNIT_TEST(TestDecodeBitsToBBox) {
        TBoundingBoxLL trueBBoxes[] = {
            {{0., 45.}, {45., 90.}},
            {{37.265625, 55.72265625}, {37.6171875, 55.8984375}},
            {{37.61581420898, 55.7487487793}, {37.6171875, 55.75012207031}},
            {{37.61650085449, 55.74995040894}, {37.61684417725, 55.75012207031}},
            {{37.61666681617, 55.74999986216}, {37.61666715145, 55.7500000298}}};
        for (auto i : xrange(Y_ARRAY_SIZE(trueBBoxes))) {
            ui8 precision = precisions[i];
            AssertDoubleBBoxesEqual(
                DecodeToBoundingBox(mskGeoHashBits[precision], precision),
                trueBBoxes[i]);
        }
    }

    Y_UNIT_TEST(TestDecodeStringToBBox) {
        TBoundingBoxLL trueBBoxes[] = {
            {{0., 45.}, {45., 90.}},
            {{30.234375, 59.94140625}, {30.5859375, 60.1171875}},
            {{30.31539916992, 59.94964599609}, {30.31677246094, 59.95101928711}},
            {{30.31642913818, 59.94998931885}, {30.31677246094, 59.95016098022}},
            {{30.31666684896, 59.94999988005}, {30.31666718423, 59.95000004768}}};
        for (auto i : xrange(Y_ARRAY_SIZE(trueBBoxes))) {
            ui8 precision = precisions[i];
            AssertDoubleBBoxesEqual(
                DecodeToBoundingBox(spbGeoHashes[precision]),
                trueBBoxes[i]);
        }
    }
}

Y_UNIT_TEST_SUITE(TNeighbourTest) {
    TString mskNeighbours12[8] = {
        "ucftpuzx7c7b",
        "ucftpuzx7ck0",
        "ucftpuzx7chp",
        "ucftpuzx7chn",
        "ucftpuzx7c5y",
        "ucftpuzx7c5w",
        "ucftpuzx7c5x",
        "ucftpuzx7c78"};

    TString mskNeighbours8[8] = {
        "ucftpvp8",
        "ucftpvpb",
        "ucftpuzz",
        "ucftpuzy",
        "ucftpuzw",
        "ucftpuzq",
        "ucftpuzr",
        "ucftpvp2"};

    TString hardGeoHash = "b58n2jbh0pb4";
    TString hardNeighbours12[8] = {
        "b58n2jbh0pb5",
        "b58n2jbh0pb7",
        "b58n2jbh0pb6",
        "b58n2jbh0pb3",
        "b58n2jbh0pb1",
        "zgxyrvzupzzc",
        "zgxyrvzupzzf",
        "zgxyrvzupzzg"};

    TString hardNeighbours8[8] = {
        "b58n2jbj",
        "b58n2jbm",
        "b58n2jbk",
        "b58n2jb7",
        "b58n2jb5",
        "zgxyrvzg",
        "zgxyrvzu",
        "zgxyrvzv"};

    Y_UNIT_TEST(TestGetNeighbourSimple) {
        for (auto i : xrange(8)) {
            auto neghb = GetNeighbour(mskGeoHashes[12], directions[i]);
            UNIT_ASSERT_STRINGS_EQUAL(neghb.GetRef(), mskNeighbours12[i]);
        }

        for (auto i : xrange(8)) {
            UNIT_ASSERT_STRINGS_EQUAL(GetNeighbour(mskGeoHashes[8], directions[i]).GetRef(), mskNeighbours8[i]);
        }
    }

    Y_UNIT_TEST(TestGetNeighbourHard) {
        for (auto i : xrange(8)) {
            UNIT_ASSERT_STRINGS_EQUAL(GetNeighbour(hardGeoHash, directions[i]).GetRef(), hardNeighbours12[i]);
        }

        auto hardGeoHash8 = hardGeoHash.substr(0, 8);
        for (auto i : xrange(8)) {
            UNIT_ASSERT_STRINGS_EQUAL(GetNeighbour(hardGeoHash8, directions[i]).GetRef(), hardNeighbours8[i]);
        }
    }

    Y_UNIT_TEST(TestGetNeighboursSimple) {
        auto neighbours = GetNeighbours(mskGeoHashes[12]);
        for (auto i : xrange(8)) {
            UNIT_ASSERT_STRINGS_EQUAL(neighbours[directions[i]].GetRef(), mskNeighbours12[i]);
        }

        neighbours = GetNeighbours(mskGeoHashes[8]);
        for (auto i : xrange(8)) {
            UNIT_ASSERT_STRINGS_EQUAL(neighbours[directions[i]].GetRef(), mskNeighbours8[i]);
        }
    }

    Y_UNIT_TEST(TestGetNeighboursHard) {
        auto neighbours = GetNeighbours(hardGeoHash);
        for (auto i : xrange(8)) {
            UNIT_ASSERT_STRINGS_EQUAL(neighbours[directions[i]].GetRef(), hardNeighbours12[i]);
        }

        neighbours = GetNeighbours(hardGeoHash.substr(0, 8));
        for (auto i : xrange(8)) {
            UNIT_ASSERT_STRINGS_EQUAL(neighbours[directions[i]].GetRef(), hardNeighbours8[i]);
        }
    }

    Y_UNIT_TEST(TestNeighbourOverflow) {
        TString geoHash = "u";

        UNIT_ASSERT(GetNeighbour(geoHash, EDirection::NORTH).Empty());
        UNIT_ASSERT(GetNeighbour(geoHash, EDirection::NORTH_WEST).Empty());
        UNIT_ASSERT(GetNeighbour(geoHash, EDirection::NORTH_EAST).Empty());
        UNIT_ASSERT(GetNeighbour(geoHash, EDirection::WEST).Defined());
        UNIT_ASSERT(GetNeighbour(geoHash, EDirection::EAST).Defined());
    }

    Y_UNIT_TEST(TestGetNeighboursOverflow) {
        TString geoHash = "u";
        auto neighbours = GetNeighbours(geoHash);

        UNIT_ASSERT(neighbours[EDirection::NORTH].Empty());
        UNIT_ASSERT(neighbours[EDirection::WEST].Defined());
    }

    Y_UNIT_TEST(TestNeighbourNoOverflow) {
        ui64 geoBits = 26; // == u;

        for (auto i : xrange(8)) {
            Y_UNUSED(i);
            geoBits = GetNeighbour(geoBits, EDirection::WEST, 1).GetRef();
        }
        UNIT_ASSERT_EQUAL(geoBits, 26);
    }
}
