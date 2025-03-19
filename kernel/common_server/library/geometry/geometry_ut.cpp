#include "algorithms.h"
#include "polyline.h"
#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/ymath.h>

namespace {

    bool AtFirst = true;

    void InitLog(ui32 logLevel = TLOG_DEBUG) {
        if (AtFirst)
            DoInitGlobalLog("cout", logLevel, false, false);
        AtFirst = false;
    }
}

Y_UNIT_TEST_SUITE(TRtyGeometryTest) {

    Y_UNIT_TEST(AreasIntersection) {
        TVector<TSimpleCoord> coords1;
        CHECK_WITH_LOG(TSimpleCoord::DeserializeVector("0 0 10 0 10 5 0 5 0 0", coords1));
        TVector<TSimpleCoord> coords2;
        CHECK_WITH_LOG(TSimpleCoord::DeserializeVector("2 5 8 5 5 0 2 5", coords2));
        TPolyLine<TSimpleCoord> line1(coords1);
        TPolyLine<TSimpleCoord> line2(coords2);
        TVector<TPolyLine<TSimpleCoord>> areas = line1.AreasIntersection(line2, true, true);
        CHECK_WITH_LOG(areas.size() == 1);
        for (auto&& i : areas) {
            INFO_LOG << i.SerializeToString() << Endl;
            CHECK_WITH_LOG(i.GetCoords()[0].GetLengthTo(TSimpleCoord(2, 5)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[1].GetLengthTo(TSimpleCoord(8, 5)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[2].GetLengthTo(TSimpleCoord(5, 0)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[3].GetLengthTo(TSimpleCoord(2, 5)) < 1e-5);
        }
    }

    Y_UNIT_TEST(AreasIntersection1) {
        TVector<TSimpleCoord> coords1;
        CHECK_WITH_LOG(TSimpleCoord::DeserializeVector("0 0 10 0 10 5 0 5 0 0", coords1));
        TVector<TSimpleCoord> coords2;
        CHECK_WITH_LOG(TSimpleCoord::DeserializeVector("-1 10 11 10 5 0 -1 10", coords2));
        TPolyLine<TSimpleCoord> line1(coords1);
        TPolyLine<TSimpleCoord> line2(coords2);
        TVector<TPolyLine<TSimpleCoord>> areas = line1.AreasIntersection(line2, true, true);
        CHECK_WITH_LOG(areas.size() == 1);
        for (auto&& i : areas) {
            INFO_LOG << i.SerializeToString() << Endl;
            CHECK_WITH_LOG(i.GetCoords()[0].GetLengthTo(TSimpleCoord(5, 0)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[1].GetLengthTo(TSimpleCoord(8, 5)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[2].GetLengthTo(TSimpleCoord(2, 5)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[3].GetLengthTo(TSimpleCoord(5, 0)) < 1e-5);
        }
    }

    Y_UNIT_TEST(AreasIntersection2) {
        TVector<TSimpleCoord> coords1;
        CHECK_WITH_LOG(TSimpleCoord::DeserializeVector("0 0 10 0 10 5 0 5 0 0", coords1));
        TVector<TSimpleCoord> coords2;
        CHECK_WITH_LOG(TSimpleCoord::DeserializeVector("0 10 10 10 5 -2.5 0 10", coords2));
        TPolyLine<TSimpleCoord> line1(coords1);
        TPolyLine<TSimpleCoord> line2(coords2);
        TVector<TPolyLine<TSimpleCoord>> areas = line1.AreasIntersection(line2, true, true);
        CHECK_WITH_LOG(areas.size() == 1);
        for (auto&& i : areas) {
            INFO_LOG << i.SerializeToString() << Endl;
            CHECK_WITH_LOG(i.GetCoords()[0].GetLengthTo(TSimpleCoord(4, 0)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[1].GetLengthTo(TSimpleCoord(6, 0)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[2].GetLengthTo(TSimpleCoord(8, 5)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[3].GetLengthTo(TSimpleCoord(2, 5)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[4].GetLengthTo(TSimpleCoord(4, 0)) < 1e-5);
        }
    }

    Y_UNIT_TEST(AreasIntersection3) {
        TVector<TSimpleCoord> coords1;
        CHECK_WITH_LOG(TSimpleCoord::DeserializeVector("0 0 20 0 20 15 15 15 15 5 5 5 5 15 0 15 0 0", coords1));
        TVector<TSimpleCoord> coords2;
        CHECK_WITH_LOG(TSimpleCoord::DeserializeVector("3 3 17 3 17 30 3 30 3 3", coords2));
        TPolyLine<TSimpleCoord> line1(coords1);
        TPolyLine<TSimpleCoord> line2(coords2);
        TVector<TPolyLine<TSimpleCoord>> areas = line1.AreasIntersection(line2, true, true);
        CHECK_WITH_LOG(areas.size() == 1);
        for (auto&& i : areas) {
            INFO_LOG << i.SerializeToString() << Endl;
            CHECK_WITH_LOG(i.GetCoords()[0].GetLengthTo(TSimpleCoord(3, 15)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[1].GetLengthTo(TSimpleCoord(3, 3)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[2].GetLengthTo(TSimpleCoord(17, 3)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[3].GetLengthTo(TSimpleCoord(17, 15)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[4].GetLengthTo(TSimpleCoord(15, 15)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[5].GetLengthTo(TSimpleCoord(15, 5)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[6].GetLengthTo(TSimpleCoord(5, 5)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[7].GetLengthTo(TSimpleCoord(5, 15)) < 1e-5);
            CHECK_WITH_LOG(i.GetCoords()[8].GetLengthTo(TSimpleCoord(3, 15)) < 1e-5);
        }
    }

    Y_UNIT_TEST(AreasIntersection4) {
        TVector<TSimpleCoord> coords1;
        CHECK_WITH_LOG(TSimpleCoord::DeserializeVector("0 0 20 0 20 15 15 15 15 5 5 5 5 15 0 15 0 0", coords1));
        TVector<TSimpleCoord> coords2;
        CHECK_WITH_LOG(TSimpleCoord::DeserializeVector("3 3 7 3 7 20 13 20 13 3 17 3 17 30 3 30 3 3", coords2));
        TPolyLine<TSimpleCoord> line1(coords1);
        TPolyLine<TSimpleCoord> line2(coords2);
        TVector<TPolyLine<TSimpleCoord>> areas = line1.AreasIntersection(line2, true, true);
        CHECK_WITH_LOG(areas.size() == 2);
        for (auto&& i : areas) {
            INFO_LOG << i.SerializeToString() << Endl;
        }
        CHECK_WITH_LOG(areas[0].GetCoords()[0].GetLengthTo(TSimpleCoord(13, 5)) < 1e-5);
        CHECK_WITH_LOG(areas[0].GetCoords()[1].GetLengthTo(TSimpleCoord(13, 3)) < 1e-5);
        CHECK_WITH_LOG(areas[0].GetCoords()[2].GetLengthTo(TSimpleCoord(17, 3)) < 1e-5);
        CHECK_WITH_LOG(areas[0].GetCoords()[3].GetLengthTo(TSimpleCoord(17, 15)) < 1e-5);
        CHECK_WITH_LOG(areas[0].GetCoords()[4].GetLengthTo(TSimpleCoord(15, 15)) < 1e-5);
        CHECK_WITH_LOG(areas[0].GetCoords()[5].GetLengthTo(TSimpleCoord(15, 5)) < 1e-5);
        CHECK_WITH_LOG(areas[0].GetCoords()[6].GetLengthTo(TSimpleCoord(13, 5)) < 1e-5);

        CHECK_WITH_LOG(areas[1].GetCoords()[0].GetLengthTo(TSimpleCoord(3, 15)) < 1e-5);
        CHECK_WITH_LOG(areas[1].GetCoords()[1].GetLengthTo(TSimpleCoord(3, 3)) < 1e-5);
        CHECK_WITH_LOG(areas[1].GetCoords()[2].GetLengthTo(TSimpleCoord(7, 3)) < 1e-5);
        CHECK_WITH_LOG(areas[1].GetCoords()[3].GetLengthTo(TSimpleCoord(7, 5)) < 1e-5);
        CHECK_WITH_LOG(areas[1].GetCoords()[4].GetLengthTo(TSimpleCoord(5, 5)) < 1e-5);
        CHECK_WITH_LOG(areas[1].GetCoords()[5].GetLengthTo(TSimpleCoord(5, 15)) < 1e-5);
        CHECK_WITH_LOG(areas[1].GetCoords()[6].GetLengthTo(TSimpleCoord(3, 15)) < 1e-5);
    }

    Y_UNIT_TEST(AreaCalc) {
        TVector<TGeoCoord> coords1;
        CHECK_WITH_LOG(TGeoCoord::DeserializeVector("37.40648985561972 55.96263661865749 37.40975142178183 55.96268476237254 37.411232001158055 55.96240793518962 37.41091013607627 55.96158947797509 37.410159117552084 55.960722857273154 37.40657568630819 55.960421942749385 37.404258257719334 55.95996454817366 37.40078211483601 55.95996454817366 37.400267130705174 55.9610719151774 37.40221977886801 55.96202278101854 37.40530968365317 55.962612546777414 37.40648985561972 55.96263661865749", coords1));
        TVector<TGeoCoord> coords2;
        CHECK_WITH_LOG(TGeoCoord::DeserializeVector("37.40325392 55.96124612 37.4037496 55.96012876 37.40957321 55.9609392 37.40907968 55.96204653 37.40325392 55.96124612", coords2));
        TPolyLine<TGeoCoord> line1(coords1);
        TPolyLine<TGeoCoord> line2(coords2);
        CHECK_WITH_LOG(line1.GetArea() > line2.GetArea());
    }

    Y_UNIT_TEST(Deserialization) {
        TVector<TGeoCoord> v;
        CHECK_WITH_LOG(TGeoCoord::DeserializeVector(" 1  1  2  2   3   3  ", v));
        CHECK_WITH_LOG(v.size() == 3);
        CHECK_WITH_LOG(Abs(v[0].X - 1) < 1e-6);
        CHECK_WITH_LOG(Abs(v[0].Y - 1) < 1e-6);
        CHECK_WITH_LOG(Abs(v[1].X - 2) < 1e-6);
        CHECK_WITH_LOG(Abs(v[1].Y - 2) < 1e-6);
        CHECK_WITH_LOG(Abs(v[2].X - 3) < 1e-6);
        CHECK_WITH_LOG(Abs(v[2].Y - 3) < 1e-6);
        CHECK_WITH_LOG(TGeoCoord::DeserializeVectorByPoints(" 1  1 , 2  2 ,  3   3  ", v));
        CHECK_WITH_LOG(v.size() == 3);
        CHECK_WITH_LOG(Abs(v[0].X - 1) < 1e-6);
        CHECK_WITH_LOG(Abs(v[0].Y - 1) < 1e-6);
        CHECK_WITH_LOG(Abs(v[1].X - 2) < 1e-6);
        CHECK_WITH_LOG(Abs(v[1].Y - 2) < 1e-6);
        CHECK_WITH_LOG(Abs(v[2].X - 3) < 1e-6);
        CHECK_WITH_LOG(Abs(v[2].Y - 3) < 1e-6);
        CHECK_WITH_LOG(!TGeoCoord::DeserializeVectorByPoints(" 1  1 , 2  2 ,  3   3 , ", v));
        CHECK_WITH_LOG(TGeoCoord::DeserializeVectorByPoints(" 1  1 ", v));
    }

    Y_UNIT_TEST(DistanceGrowing) {
        TGeoRect rect(-180, -90, 180, 90);
        TGeoRect rectFull(-180, -90, 180, 90);
        rect.GrowDistance(-1000);
        CHECK_WITH_LOG(rect.Max.X > rect.Min.X);
        CHECK_WITH_LOG(rect.Max.Y > rect.Min.Y);
        CHECK_WITH_LOG(rect.Max.Y < rectFull.Max.Y);
        CHECK_WITH_LOG(rect.Min.Y > rectFull.Min.Y);
        CHECK_WITH_LOG(rect.Max.X <= rectFull.Max.X);
        CHECK_WITH_LOG(rect.Min.X >= rectFull.Min.X);
        CHECK_WITH_LOG(rectFull.ContainLB(rect));
        CHECK_WITH_LOG((TGeoCoord::CalcRectArea(rect.Min, rect.Max) / TGeoCoord::CalcRectArea(rectFull.Min, rectFull.Max) - 1) < 1e-5);
    }

    Y_UNIT_TEST(CoordsNormalization) {
        {
            TGeoCoord c(361, 98);
            c.NormalizeCoord();
            CHECK_WITH_LOG(c.X == 1);
            CHECK_WITH_LOG(c.Y == 82);
        }
        {
            TGeoCoord c(-361, -98);
            c.NormalizeCoord();
            CHECK_WITH_LOG(c.X == -1);
            CHECK_WITH_LOG(c.Y == -82);
        }
        {
            TGeoCoord c(-181, -89);
            c.NormalizeCoord();
            CHECK_WITH_LOG(c.X == 179);
            CHECK_WITH_LOG(c.Y == -89);
        }
        {
            TGeoCoord c(181, 89);
            c.NormalizeCoord();
            CHECK_WITH_LOG(c.X == -179);
            CHECK_WITH_LOG(c.Y == 89);
        }
    }

    Y_UNIT_TEST(SimpleCoordParsing) {
        {
            TGeoCoord c("  11.1   33   ");
            UNIT_ASSERT_EQUAL(c.X, 11.1);
            UNIT_ASSERT_EQUAL(c.Y, 33);
        }
        {
            TGeoCoord c("  11.1   33");
            UNIT_ASSERT_EQUAL(c.X, 11.1);
            UNIT_ASSERT_EQUAL(c.Y, 33);
        }
        {
            TGeoCoord c("11.33   33");
            UNIT_ASSERT_EQUAL(c.X, 11.33);
            UNIT_ASSERT_EQUAL(c.Y, 33);
        }
        {
            TGeoCoord c("11 3");
            UNIT_ASSERT_EQUAL(c.X, 11);
            UNIT_ASSERT_EQUAL(c.Y, 3);
        }
    }

    template <class TPolyLine>
    bool CheckInflateLines(const TPolyLine& lineFrom, const TPolyLine& lineTo, const double delta) {
        for (ui32 i = 0; i < lineFrom.Size() - 1; ++i) {
            UNIT_ASSERT((delta > 0) == lineTo.IsPointInternal(lineFrom[i]));
            double d = lineFrom[i].GetLengthTo(lineTo[i]) - Abs(delta);
            DEBUG_LOG << d << Endl;
            UNIT_ASSERT(Abs(d) < 1e-2);
        }
        return true;
    }

    Y_UNIT_TEST(Functions) {
        TCoord<double> c0(0, 0);
        TCoord<double> c1(1, 0);
        TCoord<double> c2(1, 1);

        UNIT_ASSERT(std::abs(Angle(c1, c0, c1)) < OperationPrecision);
        double a0 = Angle(c1, c0, c2) * 180 / M_PI;
        UNIT_ASSERT(std::abs(a0 - 45) < OperationPrecision);
    }

    Y_UNIT_TEST(GeoFunctions) {
        {
            TGeoCoord c0(36, 55);
            TGeoCoord c1(36.1, 55.1);
            TGeoCoord c2(36.1, 54.85);

            UNIT_ASSERT(c0.GetScalarProduct(TGeoCoord(36.1, 55.1), TGeoCoord(36.1, 54.85)) > 0);
            double s = c0.GetSin(c1, c0, c1);
            double c = c0.GetCos(c1, c0, c1);
            CHECK_WITH_LOG(std::abs(s * s + c * c - 1) < 1e-5);
            UNIT_ASSERT(std::abs(Angle(c1, c0, c1)) < OperationPrecision);
            double a0 = Angle(c1, c0, c2) * 180 / M_PI;
            UNIT_ASSERT(std::abs(a0 + 70.5451) < OperationPrecision);

            TGeoCoord cNorth(36, 56);
            TGeoCoord cEast(37, 55);
            double a1 = Angle(cNorth, c0, cEast) * 180 / M_PI;
            UNIT_ASSERT(std::abs(a1 + 90) < OperationPrecision);
        }

        {
            TGeoCoord c0(37.004209512999999, 55.421898288000001);
            TGeoCoord cl(37.004583810011383, 55.421538769708889);
            TGeoCoord cr(37.004583810011383, 55.421538769708889);
            double a = Angle(cl, c0, cr) * 180 / M_PI;
            CHECK_WITH_LOG(std::abs(a) < OperationPrecision) << a << Endl;
        }

    }

    Y_UNIT_TEST(CheckMutualPosition) {
        TCoord<double> c0(0, 0);
        TCoord<double> cBase(1, 0);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, TCoord<double>(1, 10)) == EMutualPosition::mpLeft);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, TCoord<double>(1, -10)) == EMutualPosition::mpRight);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, TCoord<double>(100, 0)) == EMutualPosition::mpSame);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, TCoord<double>(-100, 0)) == EMutualPosition::mpReverse);
        UNIT_ASSERT(c0.GetMutualPosition(c0, TCoord<double>(100, 0)) == EMutualPosition::mpCantDetect);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, c0) == EMutualPosition::mpCantDetect);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, cBase) == EMutualPosition::mpSame);
    }

    Y_UNIT_TEST(CheckGeoMutualPosition) {
        TGeoCoord c0(36, 55);
        TGeoCoord cBase(36.6, 55);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, TGeoCoord(36.6, 55.1)) == EMutualPosition::mpLeft);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, TGeoCoord(36.6, 54.9)) == EMutualPosition::mpRight);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, TGeoCoord(36.8, 55)) == EMutualPosition::mpSame);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, TGeoCoord(35.5, 55)) == EMutualPosition::mpReverse);
        UNIT_ASSERT(c0.GetMutualPosition(c0, TGeoCoord(100, 0)) == EMutualPosition::mpCantDetect);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, c0) == EMutualPosition::mpCantDetect);
        UNIT_ASSERT(c0.GetMutualPosition(cBase, cBase) == EMutualPosition::mpSame);
    }

    Y_UNIT_TEST(InflateRect) {
        InitLog(TLOG_INFO);
        TCoord<double> c0(0, 0);
        TCoord<double> c1(1, 0);
        TCoord<double> c2(1, 1);
        TCoord<double> c3(0, 1);
        {
            TPolyLine<TCoord<double>> line({c0, c1, c2, c3, c0});
            TPolyLine<TCoord<double>> line1 = line;
            UNIT_ASSERT(line.Inflate(0.1));
            UNIT_ASSERT(CheckInflateLines(line1, line, 0.1));
        }
        {
            TPolyLine<TCoord<double>> line({c0, c0, c0, c1, c1, c1, c2, c2, c2, c2, c3, c3, c3, c3, c0, c0, c0, c0});
            TPolyLine<TCoord<double>> line1 = line;
            UNIT_ASSERT(line.Inflate(0.1));
            UNIT_ASSERT(CheckInflateLines(line1, line, 0.1));
            UNIT_ASSERT(line.Inflate(-0.2));
            UNIT_ASSERT(CheckInflateLines(line1, line, -0.1));
        }
    }

    Y_UNIT_TEST(InflateConvex) {
        InitLog(TLOG_INFO);
        TGeoCoord c0(37.6272219683, 55.7453421563);
        TGeoCoord c1(37.627166, 55.745427);
        TGeoCoord c2(37.6290916602, 55.7463465648);
        TGeoCoord c3(37.6272219683, 55.7453421563);
        {
            TPolyLine<TGeoCoord> line({ c0, c1, c2, c3});
            TPolyLine<TGeoCoord> line1 = line;
            UNIT_ASSERT(line.Inflate(200));
            UNIT_ASSERT(CheckInflateLines(line1, line, 200));
        }
    }

    Y_UNIT_TEST(InflateNotConvex) {
        InitLog(TLOG_INFO);
        {
            TCoord<double> c0(0, 0);
            TCoord<double> c1(0.5, 0.5);
            TCoord<double> c2(0, 1);
            TCoord<double> c3(1, 1);
            TCoord<double> c4(1, 0);
            {
                TPolyLine<TCoord<double>> line({c0, c1, c2, c3, c4, c0});
                TPolyLine<TCoord<double>> line1 = line;
                UNIT_ASSERT(line.Inflate(0.2));
                UNIT_ASSERT(CheckInflateLines(line1, line, 0.2));
            }
            {
                TPolyLine<TCoord<double>> line({c0, c0, c0, c1, c1, c1, c2, c2, c2, c2, c3, c3, c3, c3, c4, c4, c4, c0, c0, c0, c0});
                TPolyLine<TCoord<double>> line1 = line;
                UNIT_ASSERT(line.Inflate(0.2));
                UNIT_ASSERT(CheckInflateLines(line1, line, 0.2));
                UNIT_ASSERT(line.Inflate(-0.4));
                UNIT_ASSERT(CheckInflateLines(line1, line, -0.2));
            }
        }
        {
            TCoord<double> c0(1, 0);
            TCoord<double> c1(1, 1);
            TCoord<double> c2(0, 1);
            TCoord<double> c3(0.5, 0.5);
            TCoord<double> c4(0, 0);
            {
                TPolyLine<TCoord<double>> line({c0, c1, c2, c3, c4, c0});
                TPolyLine<TCoord<double>> line1 = line;
                UNIT_ASSERT(line.Inflate(0.1));
                UNIT_ASSERT(CheckInflateLines(line1, line, 0.1));
            }
            {
                TPolyLine<TCoord<double>> line({c0, c0, c0, c1, c1, c1, c2, c2, c2, c2, c3, c3, c3, c3, c4, c4, c4, c0, c0, c0, c0});
                TPolyLine<TCoord<double>> line1 = line;
                UNIT_ASSERT(line.Inflate(0.1));
                UNIT_ASSERT(CheckInflateLines(line1, line, 0.1));
                UNIT_ASSERT(line.Inflate(-0.2));
                UNIT_ASSERT(CheckInflateLines(line1, line, -0.1));
            }
        }
    }

    Y_UNIT_TEST(MoveGeo) {
        TGeoCoord result;
        TGeoCoord c0(30, 30);
        CHECK_WITH_LOG(c0.BuildMoved(TGeoCoord(0.1, 0.1), 1, result));
        CHECK_WITH_LOG(Abs(c0.GetLengthTo(result) - 1) < 1e-3) << c0.GetLengthTo(result);
    }

    Y_UNIT_TEST(NormalGeo) {
        TGeoCoord c0(30, 30);
        TGeoCoord c1(30.01, 30.01);

        TGeoCoord r;
        TGeoCoord::BuildNormal(c0, c1, 1, r);
        double d = c0.GetLengthTo(c0 + r);

        TGeoCoord dc = c1 - c0;

        double scalMult = dc.X * c0.GetSigma() * r.X * c0.GetSigma() + dc.Y * r.Y;

        UNIT_ASSERT(Abs(d - 1) < 1e-4);
        UNIT_ASSERT(Abs(scalMult) < 1e-20);
    }

    Y_UNIT_TEST(InflateNotConvexGeo) {
        InitLog(TLOG_INFO);
        {
            TGeoCoord c0(30, 30);
            TGeoCoord c1(30.5, 30.5);
            TGeoCoord c2(30, 31);
            TGeoCoord c3(31, 31);
            TGeoCoord c4(31, 30);
            {
                TPolyLine<TGeoCoord> line({c0, c1, c2, c3, c4, c0});
                TPolyLine<TGeoCoord> line1 = line;
                UNIT_ASSERT(line.Inflate(0.2));
                UNIT_ASSERT(CheckInflateLines(line1, line, 0.2));
            }
            {
                TPolyLine<TGeoCoord> line({c0, c0, c0, c1, c1, c1, c2, c2, c2, c2, c3, c3, c3, c3, c4, c4, c4, c0, c0, c0, c0});
                TPolyLine<TGeoCoord> line1 = line;
                UNIT_ASSERT(line.Inflate(0.2));
                UNIT_ASSERT(CheckInflateLines(line1, line, 0.2));
                UNIT_ASSERT(line.Inflate(-0.4));
                UNIT_ASSERT(CheckInflateLines(line1, line, -0.2));
            }
        }
        {
            TGeoCoord c0(31, 30);
            TGeoCoord c1(31, 31);
            TGeoCoord c2(30, 31);
            TGeoCoord c3(30.5, 30.5);
            TGeoCoord c4(30, 30);
            {
                TPolyLine<TGeoCoord> line({c0, c1, c2, c3, c4, c0});
                TPolyLine<TGeoCoord> line1 = line;
                UNIT_ASSERT(line.Inflate(0.1));
                UNIT_ASSERT(CheckInflateLines(line1, line, 0.1));
            }
            {
                TPolyLine<TGeoCoord> line({c0, c0, c0, c1, c1, c1, c2, c2, c2, c2, c3, c3, c3, c3, c4, c4, c4, c0, c0, c0, c0});
                TPolyLine<TGeoCoord> line1 = line;
                UNIT_ASSERT(line.Inflate(0.1));
                UNIT_ASSERT(CheckInflateLines(line1, line, 0.1));
                UNIT_ASSERT(line.Inflate(-0.2));
                UNIT_ASSERT(CheckInflateLines(line1, line, -0.1));
            }
        }
    }

    Y_UNIT_TEST(CutSpherePrecisionLimits) {
        TGeoCoord center(37.906106000000001, 55.670102000000000);
        TGeoCoord c1(37.907009124755859, 55.669361114501953);
        TGeoCoord c2(37.906940460205078, 55.669456481933594);

        TGeoCoord r1, r2;
        UNIT_ASSERT(center.CrossSphere(100, c1, c2, r1, r2, 1e-3) == 1);
    }

    Y_UNIT_TEST(InflateLineTesting2) {
        TGeoCoord c1(10, 10);
        TGeoCoord c2(11, 11);

        TPolyLine<TGeoCoord> line({c1, c2});
        line.InflateLine(10);
        UNIT_ASSERT(Abs(line[0].GetLengthTo(c1) - 10) < 1e-5);
        UNIT_ASSERT(Abs(line[1].GetLengthTo(c2) - 10) < 1e-5);

        TPolyLine<TGeoCoord> line1({c1, c2});
        line1.InflateLine(-10);
        UNIT_ASSERT(Abs(line1[0].GetLengthTo(c1) - 10) < 1e-5);
        UNIT_ASSERT(Abs(line1[1].GetLengthTo(c2) - 10) < 1e-5);

        UNIT_ASSERT(Abs(line1[0].GetLengthTo(line[0]) - 20) < 1e-5);
        UNIT_ASSERT(Abs(line1[1].GetLengthTo(line[1]) - 20) < 1e-5);

    }

    Y_UNIT_TEST(InflateLineTesting) {
        TVector<TGeoCoord> coords;
        UNIT_ASSERT(TGeoCoord::DeserializeVector("10 10 11 10 12 10", coords));

        TPolyLine<TGeoCoord> line(coords);
        line.InflateLine(10);
        for (ui32 i = 0; i < coords.size(); ++i) {
            UNIT_ASSERT(Abs(coords[i].GetLengthTo(line[i]) - 10) < 1e-5);
        }

        TPolyLine<TGeoCoord> line1(coords);
        line1.InflateLine(-10);
        for (ui32 i = 0; i < coords.size(); ++i) {
            UNIT_ASSERT(Abs(coords[i].GetLengthTo(line1[i]) - 10) < 1e-5);
        }

        for (ui32 i = 0; i < coords.size(); ++i) {
            UNIT_ASSERT(Abs(line[i].GetLengthTo(line1[i]) - 20) < 1e-5);
        }

        UNIT_ASSERT(!line1.Cross(line).size());

    }

    Y_UNIT_TEST(RectsSphereAreas) {
        double areaSphere = M_PI * 4 * TGeoCoord::EarthR * TGeoCoord::EarthR;

        {
            double area = TGeoCoord::CalcRectArea(TGeoCoord(0, -90), TGeoCoord(360, 90));
            UNIT_ASSERT(Abs(area - areaSphere) < 1e-5);
        }

        {
            double area = TGeoCoord::CalcRectArea(TGeoCoord(-180, -180), TGeoCoord(180, 0));
            UNIT_ASSERT(Abs(area - areaSphere) < 1e-5);
        }

        {
            double area = TGeoCoord::CalcRectArea(TGeoCoord(-180, -270), TGeoCoord(180, 90));
            UNIT_ASSERT(Abs(area / areaSphere - 2) < 1e-5);
        }


        {
            double area = TGeoCoord::CalcRectArea(TGeoCoord(-180, 90 - 1e-3), TGeoCoord(180, 90 + 1e-3));
            UNIT_ASSERT(Abs(area / areaSphere) < 1e-4);
        }

        {
            double area = TGeoCoord::CalcRectArea(TGeoCoord(-180, -90 - 1e-3), TGeoCoord(180, -90 + 1e-3));
            UNIT_ASSERT(Abs(area / areaSphere) < 1e-4);
        }

        {
            double area = TGeoCoord::CalcRectArea(TGeoCoord(-180, 270 - 1e-3), TGeoCoord(180, 270 + 1e-3));
            UNIT_ASSERT(Abs(area / areaSphere) < 1e-4);
        }

        {
            double area = TGeoCoord::CalcRectArea(TGeoCoord(-180, 3870 - 1e-3), TGeoCoord(180, 3870 + 1e-3));
            UNIT_ASSERT(Abs(area / areaSphere) < 1e-4);
        }

        {
            for (i32 i = -4000; i < 4000; ++i) {
                double area = TGeoCoord::CalcRectArea(TGeoCoord(-180, i - 1e-3), TGeoCoord(180, i + 1e-3));
                UNIT_ASSERT(Abs(area / areaSphere) < 1e-4);
            }
        }


    }

    Y_UNIT_TEST(CutSphereReal) {
        TVector<TGeoCoord> coords;
        typename TPolyLine<TGeoCoord>::TLineInterval interval;

        CHECK_WITH_LOG(TGeoCoord::DeserializeVector("37.6364975 55.7660675 37.63628006 55.76616669 37.6361084 55.76623535", coords));
        CHECK_WITH_LOG(TPolyLine<TGeoCoord>(coords).CutSphere(TGeoCoord(37.63527298, 55.7654686), 100, interval, 0.1));

        CHECK_WITH_LOG(TGeoCoord::DeserializeVector("37.66380692 55.89621735 37.66361618 55.89627457 37.66357803 55.89628983", coords));
        CHECK_WITH_LOG(TPolyLine<TGeoCoord>(coords).CutSphere(TGeoCoord(37.66292572, 55.89546204), 100, interval, 0.1));

        CHECK_WITH_LOG(TGeoCoord::DeserializeVector("37.51062393 55.80550766 37.51074219 55.80563354", coords));
        CHECK_WITH_LOG(TPolyLine<TGeoCoord>(coords).CutSphere(TGeoCoord(37.503504, 55.807547), 500, interval, 0.1));

        CHECK_WITH_LOG(TGeoCoord::DeserializeVector("41.35977253 45.87199001 41.35521155 45.86817789 41.34299543 45.85799871 41.33882996 45.85454557 41.33769672 45.85359533 41.33660507 45.85269193 41.33625868 45.85242021 41.33604909 45.85227032 41.33578987 45.85213635 41.3355387 45.85201738 41.33521807 45.85190405 41.3349383 45.85182983 41.33468267 45.85177997 41.33438599 45.85174022 41.33402598 45.85172071 41.33381691 45.85171449 41.32976275 45.85174354 41.32122125 45.85176697 41.31515879 45.85177212 41.31276559 45.85179602 41.3122997 45.85183163 41.31190832 45.85190954 41.31157595 45.85202118 41.3111965 45.852199 41.30910706 45.85338167 41.30833942 45.85376977 41.30799354 45.8538832 41.30767448 45.85397414 41.30727631 45.85406212 41.30679768 45.85413136 41.30615663 45.85417447 41.30244082 45.85425503", coords));
        CHECK_WITH_LOG(TPolyLine<TGeoCoord>(coords).CutSphere(TGeoCoord(41.3766, 45.8491), 2785, interval, 0.1));

        CHECK_WITH_LOG(TGeoCoord::DeserializeVector("41.35977253 45.87199001 41.35521155 45.86817789 41.34299543 45.85799871 41.33882996 45.85454557 41.33769672 45.85359533 41.33660507 45.85269193 41.33625868 45.85242021 41.33604909 45.85227032 41.33578987 45.85213635 41.3355387 45.85201738 41.33521807 45.85190405 41.3349383 45.85182983 41.33468267 45.85177997 41.33438599 45.85174022 41.33402598 45.85172071 41.33381691 45.85171449 41.32976275 45.85174354 41.32122125 45.85176697 41.31515879 45.85177212 41.31276559 45.85179602 41.3122997 45.85183163 41.31190832 45.85190954 41.31157595 45.85202118 41.3111965 45.852199 41.30910706 45.85338167 41.30833942 45.85376977 41.30799354 45.8538832 41.30767448 45.85397414 41.30727631 45.85406212 41.30679768 45.85413136 41.30615663 45.85417447 41.30244082 45.85425503", coords));
        CHECK_WITH_LOG(TPolyLine<TGeoCoord>(coords).CutSphere(TGeoCoord(41.35655387918211, 45.86089702674604), 1000, interval, 0.1));
    }

    Y_UNIT_TEST(CutSphere) {
        TGeoCoord c1(33.99, 33.99);
        TGeoCoord c2(34.01, 34.01);
        TGeoCoord c3(34.02, 34.02);
        TPolyLine<TGeoCoord> l1({c1, c2, c3});
        typename TPolyLine<TGeoCoord>::TLineInterval interval;
        TGeoCoord c(34, 34);
        l1.CutSphere(c, 10, interval, 0.1);
        double d = l1.GetCoord(interval.GetFirst()).GetLengthTo(c);
        UNIT_ASSERT(::Abs<double>(d - 10) < 1e-1);
        UNIT_ASSERT(::Abs<double>(l1.GetCoord(interval.GetFinish()).GetLengthTo(c) - 10) < 1e-1);
        UNIT_ASSERT(::Abs<double>(l1.GetCoord(interval.GetFinish()).GetLengthTo(l1.GetCoord(interval.GetFirst())) - 20) < 1e-1);

        l1.CutSphere(c, 10000, interval, 0.1);
        UNIT_ASSERT(::Abs<double>(interval.GetLength() - l1.GetLength()) < 1e-1);
    }

    Y_UNIT_TEST(CutSphereSector) {
        TGeoCoord c1(33.99, 33.99);
        TGeoCoord c2(34.02, 34.02);
        TPolyLine<TGeoCoord> l1({c1, c2});
        typename TPolyLine<TGeoCoord>::TLineInterval interval;
        TGeoCoord c(34, 34);
        l1.CutSphere(c, 10, interval, 0.1);
        double d = l1.GetCoord(interval.GetFirst()).GetLengthTo(c);
        UNIT_ASSERT(::Abs<double>(d - 10) < 1e-1);
        UNIT_ASSERT(::Abs<double>(l1.GetCoord(interval.GetFinish()).GetLengthTo(c) - 10) < 1e-1);
        UNIT_ASSERT(::Abs<double>(l1.GetCoord(interval.GetFinish()).GetLengthTo(l1.GetCoord(interval.GetFirst())) - 20) < 1e-1);

        l1.CutSphere(c, 10000, interval, 0.1);
        UNIT_ASSERT(::Abs<double>(interval.GetLength() - l1.GetLength()) < 1e-1);
    }

    Y_UNIT_TEST(ProjectCoord) {
        InitLog();
        {
            TCoord<double> c1(0, 0);
            TCoord<double> c2(2, -5);

            TCoord<double> c(0.1, 0);

            double l1 = c1.GetLengthTo(c);
            double l2 = c2.GetLengthTo(c);
            double l = 0;
            c.ProjectTo(c1, c2, &l);

            UNIT_ASSERT(l < l1);
            UNIT_ASSERT(l < l2);
        }
    }

    Y_UNIT_TEST(SimpleTangent) {
        InitLog();
        TGeoCoord c1(37.83048248, 55.68769455);
        TGeoCoord c2(37.83069992, 55.68710709);
        TGeoCoord v0, v1;

        {
            TPolyLine<TGeoCoord> line({c1, c2});
            UNIT_ASSERT(line.GetTangent(0, v0));
            UNIT_ASSERT(line.GetTangent(1, v1));
            UNIT_ASSERT_EQUAL(v0, v1);
        }

        {
            TPolyLine<TGeoCoord> line({c1, c1});
            UNIT_ASSERT(!line.GetTangent(0, v0));
            UNIT_ASSERT(!line.GetTangent(1, v1));
        }
    }

    Y_UNIT_TEST(SpecialTangent) {
        InitLog();
        TGeoCoord c1(37.83048248, 55.68769455);
        TGeoCoord c2(37.83069992, 55.68710709);
        TGeoCoord v0, v1;

        {
            TPolyLine<TGeoCoord> line({c1, c1, c1, c2, c2, c2});
            UNIT_ASSERT(line.GetTangent(0, v0));
            UNIT_ASSERT(line.GetTangent(1, v1));
            UNIT_ASSERT_EQUAL(v0, v1);
            UNIT_ASSERT(v0.SimpleLength() > 6e-4);
        }
    }

    Y_UNIT_TEST(ProjectCoordSmallDiffGEO) {
        InitLog();
        {
            TGeoCoord c1(37.83048248, 55.68769455);
            TGeoCoord c2(37.83069992, 55.68710709);

            TGeoCoord c = c1 + TGeoCoord(1, 0) * 0.00001;

            double l1 = c1.GetLengthTo(c);
            double l2 = c2.GetLengthTo(c);
            double l = 0;

            c.ProjectTo(c1, c2, &l);

            UNIT_ASSERT(l < l1);
            UNIT_ASSERT(l < l2);
        }
    }

    struct TBoolFlag {
        bool Flag = false;

        TBoolFlag() {

        }

        TBoolFlag(bool flag)
            : Flag(flag)
        {

        }

        bool Follow() const {
            return Flag;
        }

        double GetValue() const {
            return Flag ? 1 : 0;
        }
    };

    Y_UNIT_TEST(ReverseTest) {
        TVector<TCoord<double>> coords1, coords2;
        for (ui32 i = 0; i <= 180; ++i) {
            auto c = TCoord<double>(cos(i * M_PI / 360), sin(i * M_PI / 360));
            coords1.push_back(c);
            coords2.insert(coords2.begin(), c);
        }

        TPolyLine<TCoord<double>> line1(coords1);
        TPolyLine<TCoord<double>> line2(coords2);
        for (i32 i = 0; i <= 180; ++i) {
            auto p = line1.GetPosition(i);
            INFO_LOG << line1[i].ToString() << " == " << line1.GetCoord(p).ToString() << Endl;
            UNIT_ASSERT(line1[i].Compare(line1.GetCoord(p), 1e-7));
            INFO_LOG << p.Reverse(line1).ToString() << Endl;
            INFO_LOG << p.Reverse(line1).Reverse(line2).ToString() << Endl;
            INFO_LOG << p.ToString() << Endl;
            UNIT_ASSERT(i == 180 || p.Reverse(line1).Reverse(line2).Compare(p, 1e-7));
            INFO_LOG << line1[i].ToString() << " == " << line2.GetCoord(p.Reverse(line1)).ToString() << Endl;
            UNIT_ASSERT(line1[i].Compare(line2.GetCoord(p.Reverse(line1)), 1e-7));
        }

        for (i32 i = 0; i < 180; ++i) {
            auto c = TCoord<double>(cos((i + 0.7) * M_PI / 360), sin((i + 0.7) * M_PI / 360));
            TPolyLine<TCoord<double>>::TPosition p;
            TCoord<double> c1;
            UNIT_ASSERT(line1.MatchSimple(c, 1e-5, p, &c1));
            INFO_LOG << c1.ToString() << " == " << line1.GetCoord(p).ToString() << Endl;
            UNIT_ASSERT(c1.Compare(line1.GetCoord(p), 1e-7));
            INFO_LOG << p.Reverse(line1).ToString() << Endl;
            INFO_LOG << p.Reverse(line1).Reverse(line2).ToString() << Endl;
            INFO_LOG << p.ToString() << Endl;
            UNIT_ASSERT(p.Reverse(line1).Reverse(line2).Compare(p, 1e-7));
            INFO_LOG << c1.ToString() << " == " << line2.GetCoord(p.Reverse(line1)).ToString() << Endl;
            UNIT_ASSERT(c1.Compare(line2.GetCoord(p.Reverse(line1)), 1e-7));
        }
    }

    struct TDoubleValue {
        double Value;

        TDoubleValue() {
            Value = 0;
        }

        TDoubleValue(const double value) {
            Value = value;
        }

        double GetValue() const {
            return Value;
        }
    };
    /*
    Y_UNIT_TEST(FollowTest) {
        InitLog();
        TVector<TCoord<double>> coords1;
        coords1.push_back(TCoord<double>(-10, 0));
        coords1.push_back(TCoord<double>(1000, 0));
        TPolyLine<TCoord<double>> line(coords1);
        TPolyLine<TCoord<double>>::TSegmentation<TBoolFlag> segm;

        {
            TPolyLine<TCoord<double>>::TPosition posFrom(10, 10, 0);
            TPolyLine<TCoord<double>>::TPosition posTo(100, 100, 0);
            segm.AddInterval(posFrom, posTo, TBoolFlag(true));
        }

        {
            TPolyLine<TCoord<double>>::TPosition posFrom(101, 101, 0);
            TPolyLine<TCoord<double>>::TPosition posTo(110, 110, 0);
            segm.AddInterval(posFrom, posTo, TBoolFlag(true));
        }

        {
            TPolyLine<TCoord<double>>::TPosition posFrom(120, 120, 0);
            TPolyLine<TCoord<double>>::TPosition posTo(130, 130, 0);
            segm.AddInterval(posFrom, posTo, TBoolFlag(true));
        }

        {
            TPolyLine<TCoord<double>>::TPosition posFrom(11, 11, 0);
            TPolyLine<TCoord<double>>::TPosition result;
            TMaybe<TPolyLine<TCoord<double>>::TPosition> realPos;
            segm.FollowPosition(line, posFrom, 0, 10, result, realPos);
            UNIT_ASSERT_EQUAL(result.GetDistance(), 130);
        }

        {
            TPolyLine<TCoord<double>>::TPosition posFrom(10, 10, 0);
            TPolyLine<TCoord<double>>::TPosition result;
            TMaybe<TPolyLine<TCoord<double>>::TPosition> realPos;
            segm.FollowPosition(line, posFrom, 0, 10, result, realPos);
            UNIT_ASSERT_EQUAL(result.GetDistance(), 130);
        }

        {
            TPolyLine<TCoord<double>>::TPosition posFrom(10, 10, 0);
            TPolyLine<TCoord<double>>::TPosition result;
            TMaybe<TPolyLine<TCoord<double>>::TPosition> realPos;
            segm.FollowPosition(line, posFrom, 0, 9, result, realPos);
            UNIT_ASSERT_EQUAL(result.GetDistance(), 110);
        }

        {
            TPolyLine<TCoord<double>>::TPosition posFrom(10, 10, 0);
            TPolyLine<TCoord<double>>::TPosition result;
            TMaybe<TPolyLine<TCoord<double>>::TPosition> realPos;
            segm.FollowPosition(line, posFrom, 0, 9, result, realPos);
            UNIT_ASSERT_EQUAL(result.GetDistance(), 110);
        }

        {
            TPolyLine<TCoord<double>>::TPosition posFrom(3, 3, 0);
            TPolyLine<TCoord<double>>::TPosition result;
            TMaybe<TPolyLine<TCoord<double>>::TPosition> realPos;
            segm.FollowPosition(line, posFrom, 5, 9, result, realPos);
            UNIT_ASSERT_EQUAL(result.GetDistance(), 3);
        }

        {
            TPolyLine<TCoord<double>>::TPosition posFrom(100, 100, 0);
            TPolyLine<TCoord<double>>::TPosition result;
            TMaybe<TPolyLine<TCoord<double>>::TPosition> realPos;
            UNIT_ASSERT_EQUAL(segm.FollowPosition(line, posFrom, 5, 1, result, realPos), TPolyLine<TCoord<double>>::TSegmentation<TBoolFlag>::frStop);
            UNIT_ASSERT_EQUAL(result.GetDistance(), 100);

            UNIT_ASSERT_EQUAL(segm.FollowPosition(line, posFrom, 0, 1, result, realPos), TPolyLine<TCoord<double>>::TSegmentation<TBoolFlag>::frOK);
            UNIT_ASSERT_EQUAL(result.GetDistance(), 110);
        }

        {
            TPolyLine<TCoord<double>>::TPosition posFrom(2, 2, 0);
            TPolyLine<TCoord<double>>::TPosition result;
            TMaybe<TPolyLine<TCoord<double>>::TPosition> realPos;
            segm.FollowPosition(line, posFrom, 0, 10, result, realPos);
            UNIT_ASSERT_EQUAL(result.GetDistance(), 130);

            segm.FollowPosition(line, posFrom, 0, 9, result, realPos);
            UNIT_ASSERT_EQUAL(result.GetDistance(), 110);
        }

        {
            TPolyLine<TCoord<double>>::TPosition posFrom(0, 0, 0);
            TPolyLine<TCoord<double>>::TPosition result;
            TMaybe<TPolyLine<TCoord<double>>::TPosition> realPos;
            segm.FollowPosition(line, posFrom, 0, 10, result, realPos);
            UNIT_ASSERT_EQUAL(result.GetDistance(), 130);

            UNIT_ASSERT_EQUAL(segm.FollowPosition(line, posFrom, 0, 9, result, realPos), TPolyLine<TCoord<double>>::TSegmentation<TBoolFlag>::frStop);
            UNIT_ASSERT_EQUAL(result.GetDistance(), 0);
        }
    }
    */

    Y_UNIT_TEST(AttachTest) {
        InitLog();
        {
            TPolyLine<TGeoCoord> line1 = TPolyLine<TGeoCoord>::BuildFromString("18 20 19 20 20 20 20 24");
            TVector<TGeoCoord> coords;
            line1.AttachMeTo(coords, false);
            CHECK_WITH_LOG(coords.size() == 4);
            CHECK_WITH_LOG(coords[3].Compare(TGeoCoord(18, 20), 1e-5));
            CHECK_WITH_LOG(coords[2].Compare(TGeoCoord(19, 20), 1e-5));
            CHECK_WITH_LOG(coords[1].Compare(TGeoCoord(20, 20), 1e-5));
            CHECK_WITH_LOG(coords[0].Compare(TGeoCoord(20, 24), 1e-5));
        }
        {
            TPolyLine<TGeoCoord> line1 = TPolyLine<TGeoCoord>::BuildFromString("18 20 19 20 20 20 20 24");
            TVector<TGeoCoord> coords;
            line1.AttachMeTo(coords, true);
            CHECK_WITH_LOG(coords.size() == 4);
            CHECK_WITH_LOG(coords[0].Compare(TGeoCoord(18, 20), 1e-5));
            CHECK_WITH_LOG(coords[1].Compare(TGeoCoord(19, 20), 1e-5));
            CHECK_WITH_LOG(coords[2].Compare(TGeoCoord(20, 20), 1e-5));
            CHECK_WITH_LOG(coords[3].Compare(TGeoCoord(20, 24), 1e-5));
            TPolyLine<TGeoCoord> line2 = TPolyLine<TGeoCoord>::BuildFromString("18 20 19 20 20 20 20 24");
            line2.AttachMeTo(coords, true);
            CHECK_WITH_LOG(coords.size() == 8);
            CHECK_WITH_LOG(coords[4].Compare(TGeoCoord(18, 20), 1e-5));
            CHECK_WITH_LOG(coords[5].Compare(TGeoCoord(19, 20), 1e-5));
            CHECK_WITH_LOG(coords[6].Compare(TGeoCoord(20, 20), 1e-5));
            CHECK_WITH_LOG(coords[7].Compare(TGeoCoord(20, 24), 1e-5));
        }
        {
            TPolyLine<TGeoCoord> line1 = TPolyLine<TGeoCoord>::BuildFromString("18 20 19 20 20 20 20 24");
            TVector<TGeoCoord> coords;
            line1.AttachMeTo(coords, true);
            CHECK_WITH_LOG(coords.size() == 4);
            CHECK_WITH_LOG(coords[0].Compare(TGeoCoord(18, 20), 1e-5));
            CHECK_WITH_LOG(coords[1].Compare(TGeoCoord(19, 20), 1e-5));
            CHECK_WITH_LOG(coords[2].Compare(TGeoCoord(20, 20), 1e-5));
            CHECK_WITH_LOG(coords[3].Compare(TGeoCoord(20, 24), 1e-5));
            TPolyLine<TGeoCoord> line2 = TPolyLine<TGeoCoord>::BuildFromString("18 20 19 20 20 20 20 24");
            line2.AttachMeTo(coords, false);
            CHECK_WITH_LOG(coords.size() == 7);
            CHECK_WITH_LOG(coords[4].Compare(TGeoCoord(20, 20), 1e-5));
            CHECK_WITH_LOG(coords[5].Compare(TGeoCoord(19, 20), 1e-5));
            CHECK_WITH_LOG(coords[6].Compare(TGeoCoord(18, 20), 1e-5));
        }
    }

    Y_UNIT_TEST(CoordsInInterval) {
        InitLog();
        TPolyLine<TGeoCoord> line1 = TPolyLine<TGeoCoord>::BuildFromString("18 20 19 20 20 20 20 24");

        {
            TPolyLine<TGeoCoord>::TPosition pos = line1.GetPosition(2).MoveForward(500);

            TVector<TGeoCoord> coords = line1.GetSegmentCoords({ pos });
            CHECK_WITH_LOG(coords.size() == 1);
            CHECK_WITH_LOG(coords.front().GetLengthTo(line1.GetCoord(pos)) < 1e-5);
        }

        {
            TPolyLine<TGeoCoord>::TPosition posFrom = line1.GetPosition(2).MoveForward(500);
            TPolyLine<TGeoCoord>::TPosition posTo = line1.GetPosition(0).MoveForward(100);

            TVector<TGeoCoord> coords = line1.GetSegmentCoords(TPolyLine<TGeoCoord>::TLineInterval(posFrom, posTo));
            UNIT_ASSERT_EQUAL(coords.size(), 4);
            UNIT_ASSERT(coords[0].Compare(TGeoCoord(20, 20.004495), 1e-5));
            UNIT_ASSERT(coords[1].Compare(TGeoCoord(20, 20), 1e-5));
            UNIT_ASSERT(coords[2].Compare(TGeoCoord(19, 20), 1e-5));
            UNIT_ASSERT(coords[3].Compare(TGeoCoord(18.000956, 20), 1e-5));
        }

        {
            TVector<TGeoCoord> coords = line1.GetSegmentCoords(TPolyLine<TGeoCoord>::TLineInterval(line1.GetPosition(3), line1.GetPosition(1)));
            UNIT_ASSERT_EQUAL(coords.size(), 3);
            UNIT_ASSERT(coords[2].Compare(TGeoCoord(19, 20), 1e-5));
            UNIT_ASSERT(coords[1].Compare(TGeoCoord(20, 20), 1e-5));
            UNIT_ASSERT(coords[0].Compare(TGeoCoord(20, 24), 1e-5));
        }

        {
            TVector<TGeoCoord> coords = line1.GetSegmentCoords(TPolyLine<TGeoCoord>::TLineInterval(line1.GetPosition(2).MoveForward(100), line1.GetPosition(1)));
            UNIT_ASSERT_EQUAL(coords.size(), 3);
            UNIT_ASSERT(coords[0].Compare(TGeoCoord(20, 20.000899068297553), 1e-5));
            UNIT_ASSERT(coords[1].Compare(TGeoCoord(20, 20), 1e-5));
            UNIT_ASSERT(coords[2].Compare(TGeoCoord(19, 20), 1e-5));
        }

        {
            TVector<TGeoCoord> coords = line1.GetSegmentCoords(TPolyLine<TGeoCoord>::TLineInterval(line1.GetPosition(1), line1.GetPosition(3)));
            UNIT_ASSERT_EQUAL(coords.size(), 3);
            UNIT_ASSERT(coords[0].Compare(TGeoCoord(19, 20), 1e-5));
            UNIT_ASSERT(coords[1].Compare(TGeoCoord(20, 20), 1e-5));
            UNIT_ASSERT(coords[2].Compare(TGeoCoord(20, 24), 1e-5));
        }
    }

    Y_UNIT_TEST(CoordDelta) {
        InitLog();
        for (ui32 l = 1; l < 14; l += 5) {
            for (i32 x = -179; x < 180; ++x) {
                for (i32 y = -179; y < 180; ++y) {
                    if (Abs(y) == 90)
                        continue;
                    TGeoCoord c(x, y);
                    double dx = c.MakeDXFromDistance(l);
                    double dy = c.MakeDYFromDistance(l);
                    TGeoCoord cx = c + TGeoCoord(dx, 0);
                    UNIT_ASSERT(cx.GetLengthTo(c) < l + 1e-2);
                    UNIT_ASSERT(cx.GetLengthTo(c) > l - 1e-2);
                    TGeoCoord cy = c + TGeoCoord(0, dy);
                    UNIT_ASSERT(cy.GetLengthTo(c) < l + 1e-2);
                    UNIT_ASSERT(cy.GetLengthTo(c) > l - 1e-2);
                    cx = c - TGeoCoord(dx, 0);
                    UNIT_ASSERT(cx.GetLengthTo(c) < l + 1e-2);
                    UNIT_ASSERT(cx.GetLengthTo(c) > l - 1e-2);
                    cy = c - TGeoCoord(0, dy);
                    UNIT_ASSERT(cy.GetLengthTo(c) < l + 1e-2);
                    UNIT_ASSERT(cy.GetLengthTo(c) > l - 1e-2);
                }
            }
        }
    }

    Y_UNIT_TEST(InternalPoint) {
        InitLog();
        TVector<TCoord<double>> coords;
        coords.push_back(TCoord<double>(-100, 0));
        coords.push_back(TCoord<double>(0, 100));
        coords.push_back(TCoord<double>(100, 0));
        coords.push_back(TCoord<double>(0, -100));
        coords.push_back(TCoord<double>(-100, 0));
        TPolyLine<TCoord<double>> line(coords);

        UNIT_ASSERT(line.IsPointInternal(TCoord<double>(-99, 0)));
        UNIT_ASSERT(line.IsPointInternal(TCoord<double>(-100, 0)));
        UNIT_ASSERT(line.IsPointInternal(TCoord<double>(-93, 7)));

        for (i32 i = -110; i < 110; ++i) {
            for (i32 j = -110; j < 110; ++j) {
                bool isInt = line.IsPointInternal(TCoord<double>(i, j));
                bool isIntA = Abs(i) + Abs(j) <= 100;
                isIntA &= (i + Abs(j) != 100);
                UNIT_ASSERT_EQUAL(isInt, isIntA);
            }
        }
    }

    Y_UNIT_TEST(AdditionalRects) {
        TRect<TGeoCoord> r1(0, 0, 1, 1);
        TRect<TGeoCoord> fullRect(-100, -100, 100, 100);
        TVector<TRect<TGeoCoord>> rects = r1.GetAdditionalRects(fullRect);
        double area = 0;
        for (ui32 i = 0; i < rects.size(); ++i) {
            area += TGeoCoord::CalcRectArea(rects[i].Min, rects[i].Max);
        }
        area += TGeoCoord::CalcRectArea(r1.Min, r1.Max);
        double fullArea = TGeoCoord::CalcRectArea(fullRect.Min, fullRect.Max);
        UNIT_ASSERT(Abs(area - fullArea) < 1e-5);
    }

    Y_UNIT_TEST(AdditionalRectsSimple) {
        TRect<TGeoCoord> r1(-100, -100, 100, 100);
        TRect<TGeoCoord> fullRect(-100, -100, 100, 100);
        TVector<TRect<TGeoCoord>> rects = r1.GetAdditionalRects(fullRect);
        UNIT_ASSERT(rects.size() == 0);
    }

    Y_UNIT_TEST(AdditionalRectsSimple1) {
        TRect<TGeoCoord> r1(-90, -100, 100, 100);
        TRect<TGeoCoord> fullRect(-100, -100, 100, 100);
        TVector<TRect<TGeoCoord>> rects = r1.GetAdditionalRects(fullRect);
        UNIT_ASSERT(rects.size() == 1);
        double sAdditional = TGeoCoord::CalcRectArea(rects.front().Min, rects.front().Max);
        double sFull = TGeoCoord::CalcRectArea(fullRect.Min, fullRect.Max);
        double sBase = TGeoCoord::CalcRectArea(r1.Min, r1.Max);
        UNIT_ASSERT(Abs(sAdditional + sBase - sFull) < 1e-1);
    }

    Y_UNIT_TEST(CrossRects) {
        TRect<TGeoCoord> r1(0, 0, 1, 1);
        UNIT_ASSERT(r1.ContainLB(TRect<TGeoCoord>(0, 0, 0, 0)));
        UNIT_ASSERT(!r1.ContainLB(TRect<TGeoCoord>(1, 1, 1, 1)));
        UNIT_ASSERT(!r1.ContainLB(TRect<TGeoCoord>(1, 0.5, 1, 0.5)));
        UNIT_ASSERT(!r1.ContainLB(TRect<TGeoCoord>(0.5, 1, 0.5, 1)));
        UNIT_ASSERT(!r1.ContainLB(TRect<TGeoCoord>(1, 0, 1, 0)));
        UNIT_ASSERT(!r1.ContainLB(TRect<TGeoCoord>(0, 1, 0, 1)));
        UNIT_ASSERT(r1.ContainLB(TRect<TGeoCoord>(0, 0, 1, 0.5)));
        UNIT_ASSERT(r1.ContainLB(TRect<TGeoCoord>(0.5, 0.5, 1, 1)));
        UNIT_ASSERT(r1.ContainLB(TRect<TGeoCoord>(0.5, 0.5, 1, 0.75)));
    }

    Y_UNIT_TEST(CrossPolyLines) {
        TVector<TCoord<double>> coords1;
        coords1.push_back(TCoord<double>(-100, 0));
        coords1.push_back(TCoord<double>(0, 100));
        coords1.push_back(TCoord<double>(100, 0));
        coords1.push_back(TCoord<double>(0, -100));
        coords1.push_back(TCoord<double>(-100, 0));
        TPolyLine<TCoord<double>> line1(coords1);

        TVector<TCoord<double>> coords2;
        coords2.push_back(TCoord<double>(-100, 100));
        coords2.push_back(TCoord<double>(0, 0));
        coords2.push_back(TCoord<double>(-100, -100));
        coords2.push_back(TCoord<double>(100, -100));
        coords2.push_back(TCoord<double>(0, 0));
        coords2.push_back(TCoord<double>(100, 0));
        TPolyLine<TCoord<double>> line2(coords2);

        TSet<TPolyLine<TCoord<double>>::TPosition> points = line1.Cross(line2);
        TVector<TCoord<double>> coords;
        for (auto&& i : points) {
            coords.push_back(line2.GetCoord(i));
        }

        UNIT_ASSERT_EQUAL(coords.size(), 5);
        UNIT_ASSERT(coords[0].GetLengthTo(TCoord<double>(-50, 50)) < 1e-5);
        UNIT_ASSERT(coords[1].GetLengthTo(TCoord<double>(-50, -50)) < 1e-5);
        UNIT_ASSERT(coords[2].GetLengthTo(TCoord<double>(0, -100)) < 1e-5);
        UNIT_ASSERT(coords[3].GetLengthTo(TCoord<double>(50, -50)) < 1e-5);
        UNIT_ASSERT(coords[4].GetLengthTo(TCoord<double>(100, 0)) < 1e-5);

    }

    template<class TCoord>
    bool CheckEnvelope(const TPolyLine<TCoord>& line) {
        const TVector<TCoord>& coords = line.GetCoords();
        for (ui32 i = 1; i < coords.size(); ++i) {
            TCoord p0 = coords[i -1];
            TCoord p1 = coords[i];
            for (ui32 j = 0; j < coords.size(); ++j) {
                if (j == i || j == i - 1)
                    continue;
                TCoord p2 = coords[j];
                if (p0.GetVectorProduct(p1, p2) > 0) {
                    return false;
                }
            }
        }
        return true;
    }

    Y_UNIT_TEST(BuildConvexEnvelope) {
        InitLog(TLOG_INFO);
        {
            TVector<TCoord<double>> coords;
            coords.push_back(TCoord<double>(0, 0));
            coords.push_back(TCoord<double>(0.5, 0.5));
            coords.push_back(TCoord<double>(1, 1));
            coords.push_back(TCoord<double>(0, 1));
            coords.push_back(TCoord<double>(1, 0));
            coords.push_back(TCoord<double>(0.5, 1));

            TPolyLine<TCoord<double>> line = TPolyLine<TCoord<double>>::GetConvexEnvelope(coords);
            UNIT_ASSERT(CheckEnvelope<TCoord<double>>(line));
        }
        {
            TVector<TGeoCoord> coords;
            coords.push_back(TGeoCoord(37.5879189725, 55.7338422607));
            coords.push_back(TGeoCoord(37.585529, 55.732869));
            coords.push_back(TGeoCoord(37.587874, 55.73367));
            coords.push_back(TGeoCoord(37.587874, 55.73367));
            coords.push_back(TGeoCoord(37.58863, 55.73405));
            coords.push_back(TGeoCoord(37.58863, 55.73405));

            TPolyLine<TGeoCoord> line = TPolyLine<TGeoCoord>::GetConvexEnvelope(coords);
            UNIT_ASSERT(CheckEnvelope<TGeoCoord>(line));
        }
        {
            TVector<TGeoCoord> coords;
            coords.push_back(TGeoCoord(37.597423, 55.757931));
            coords.push_back(TGeoCoord(37.5984831603, 55.7549171125));
            coords.push_back(TGeoCoord(37.598106, 55.756888));
            coords.push_back(TGeoCoord(37.598564, 55.756239));

            TPolyLine<TGeoCoord> line = TPolyLine<TGeoCoord>::GetConvexEnvelope(coords);
            TString s = line.SerializeToString();
            UNIT_ASSERT(CheckEnvelope<TGeoCoord>(line));
        }
        {
            TVector<TGeoCoord> coords;
            coords.push_back(TGeoCoord(37.567348, 55.743299));
            coords.push_back(TGeoCoord(37.566595928, 55.7449349869));
            coords.push_back(TGeoCoord(37.563693599, 55.7439445577));
            coords.push_back(TGeoCoord(37.566072, 55.744632));
            coords.push_back(TGeoCoord(37.567545, 55.744596));
            coords.push_back(TGeoCoord(37.565767, 55.742904));
            coords.push_back(TGeoCoord(37.566072, 55.744632));

            TPolyLine<TGeoCoord> line = TPolyLine<TGeoCoord>::GetConvexEnvelope(coords);
            TString s = line.SerializeToString();
            UNIT_ASSERT(CheckEnvelope<TGeoCoord>(line));
        }
    }

    Y_UNIT_TEST(CalcSin) {
        {
            TGeoCoord p1(37.55, 55.0);
            TGeoCoord c(37.56, 55.0);
            TGeoCoord p2(37.57, 55.0);
            INFO_LOG << TGeoCoord::GetSin(p1, c, p2) << Endl;
            UNIT_ASSERT(Abs(TGeoCoord::GetSin(p1, c, p2)) < 1e-3);
        }
        {
            TGeoCoord p1(37.55, 55.0);
            TGeoCoord c(37.56, 55.0);
            TGeoCoord p2(37.56, 55.0);
            INFO_LOG << TGeoCoord::GetSin(p1, c, p2) << Endl;
            UNIT_ASSERT(Abs(TGeoCoord::GetSin(p1, c, p2)) < 1e-5);
            UNIT_ASSERT(Abs(TGeoCoord::GetCos(p1, c, p2) - 1) < 1e-5);
            UNIT_ASSERT(Abs(Angle(p1, c, p2)) < 1e-5);

        }
        {
            TGeoCoord p1(37.0, 55.77);
            TGeoCoord c(37.0, 55.78);
            TGeoCoord p2(37.0, 55.79);
            INFO_LOG << TGeoCoord::GetSin(p1, c, p2) << Endl;
            UNIT_ASSERT(Abs(TGeoCoord::GetSin(p1, c, p2)) < 1e-3);
        }
        {
            TGeoCoord p1(37.1, 55.77);
            TGeoCoord c(37.0, 55.77);
            TGeoCoord p2(37.0, 55.79);
            INFO_LOG << TGeoCoord::GetSin(p1, c, p2) << Endl;
            UNIT_ASSERT(Abs(1 - TGeoCoord::GetSin(p1, c, p2)) < 1e-3);
        }

        {
            TGeoCoord p1(37.59632220522151, 55.75273141218259);
            TGeoCoord c(37.597105410253874, 55.752779817803166);
            TGeoCoord p2(37.5981890226959, 55.75284032474403);
            INFO_LOG << TGeoCoord::GetSin(p1, c, p2) << Endl;
            UNIT_ASSERT(Abs(TGeoCoord::GetSin(p1, c, p2)) < 1e-1);
        }

        {
            TGeoCoord p1(37.573555592590324, 55.752517926840035);
            TGeoCoord c(37.57233250527952, 55.7521548807676);
            TGeoCoord p2(37.571066502624504, 55.75174342445012);
            INFO_LOG << TGeoCoord::GetSin(p1, c, p2) << Endl;
            UNIT_ASSERT(Abs(TGeoCoord::GetSin(p1, c, p2)) < 1e-1);
        }
    }

    Y_UNIT_TEST(GetClipping) {
        TRect<TGeoCoord> rect(TGeoCoord(0, 0), TGeoCoord(4, 3));
        UNIT_ASSERT_VALUES_EQUAL(rect.GetClipping(TGeoCoord(0, 0), TGeoCoord(4, 3)).size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(rect.GetClipping(TGeoCoord(4, 3), TGeoCoord(0, 0)).size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(rect.GetClipping(TGeoCoord(-2, 0), TGeoCoord(2, 4)).size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(rect.GetClipping(TGeoCoord(3, -1), TGeoCoord(2, 4)).size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(rect.GetClipping(TGeoCoord(2, 5), TGeoCoord(6, 1)).size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(rect.GetClipping(TGeoCoord(-2, 2), TGeoCoord(1, 5)).size(), 0);
    }
}
