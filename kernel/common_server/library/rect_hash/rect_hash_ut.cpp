#include "rect_hash.h"
#include <library/cpp/testing/unittest/registar.h>

namespace {
    void InitLog() {
        if (!GlobalLogInitialized())
            DoInitGlobalLog("cout", 7, false, false);
    }
}

Y_UNIT_TEST_SUITE(TRtyGeometryTest) {

   struct TGeoObj {
       ui64 Id = 0;
       TRect<TGeoCoord> Rect;
       TRect<TGeoCoord> GetRect() const {
           return Rect;
       };

        bool operator<(const TGeoObj& item) const {
            return Id < item.Id;
       };

       bool operator == (const TGeoObj& item) const {
           return Id == item.Id;
       };

       TGeoObj() {}

       TGeoObj(ui64 id, const TGeoCoord& coord)
           : Id(id)
           , Rect(coord)
       {}

   };

    bool CheckRectHash(const TVector<TGeoObj>& objects, const TRectHash<TGeoCoord, TGeoObj>& hash, TRect<TGeoCoord> rect) {
        for (ui32 x = 0; x < 100; ++x) {
            for (ui32 y = 0; y < 100; ++y) {
                TRect<TGeoCoord> rectCheck(TGeoCoord(rect.Min.X + x * (rect.Max.X - rect.Min.X) / 100.0, rect.Min.Y + y * (rect.Max.Y - rect.Min.Y) / 100.0));
                rectCheck.Grow(0.01);
                TSet<TGeoObj> objectsInRect;
                hash.FindObjects(rectCheck, objectsInRect);
                ui32 counter = 0;
                for (auto&& obj: objects) {
                    if (rectCheck.CrossLB(obj.Rect)) {
                        counter++;
                        if (!objectsInRect.contains(obj)) {
                            return false;
                        }
                    }
                }
            }
        }
        return true;
    }

    Y_UNIT_TEST(AddHash) {
        InitLog();
        ui64 newId = 0;
        TRect<TGeoCoord> geoRect = TRect<TGeoCoord>(TGeoCoord(37, 55), TGeoCoord(38, 56));
        TRectBuilderDefault<TGeoCoord, TGeoObj> builder;
        TRectHash<TGeoCoord, TGeoObj> hash(geoRect, builder, THashBuilderPolicyDefault<TGeoCoord, TGeoObj>(5));
        TVector<TGeoObj> objects;
        for (ui32 x = 0; x < 100; ++x) {
            for (ui32 y = 0; y < 100; ++y) {
                TGeoObj newObj;
                newObj.Id = ++newId;
                newObj.Rect = TRect<TGeoCoord>(TGeoCoord(37 + x / 100.0, 55 + y / 100.0));
                hash.AddObject(newObj);
                objects.push_back(newObj);
            }
        }

        UNIT_ASSERT(CheckRectHash(objects, hash, geoRect));
    };

    Y_UNIT_TEST(AddNormalizeHash) {
        InitLog();
        ui64 newId = 0;
        TRect<TGeoCoord> geoRect = TRect<TGeoCoord>(TGeoCoord(37, 55), TGeoCoord(38, 56));
        TRectBuilderDefault<TGeoCoord, TGeoObj> builder;
        TRectHash<TGeoCoord, TGeoObj> hash(geoRect, builder, THashBuilderPolicyDefault<TGeoCoord, TGeoObj>(20));
        TVector<TGeoObj> objects;
        for (ui32 x = 0; x < 100; ++x) {
            for (ui32 y = 0; y < 100; ++y) {
                TGeoObj newObj;
                newObj.Id = ++newId;
                newObj.Rect = TRect<TGeoCoord>(TGeoCoord(37 + x / 100.0, 55 + y / 100.0));
                hash.AddObject(newObj);
                objects.push_back(newObj);
            }
        }

        hash.GetPolicy()->SetCriticalObjectsCount(2);

        UNIT_ASSERT(CheckRectHash(objects, hash, geoRect));
    };

    Y_UNIT_TEST(AddRemoveHash) {
        InitLog();
        ui64 newId = 0;
        TRect<TGeoCoord> geoRect = TRect<TGeoCoord>(TGeoCoord(37, 55), TGeoCoord(38, 56));
        TRectBuilderDefault<TGeoCoord, TGeoObj> builder;
        TRectHash<TGeoCoord, TGeoObj> hash(geoRect, builder, THashBuilderPolicyDefault<TGeoCoord, TGeoObj>(20));
        TVector<TGeoObj> objects;
        for (ui32 x = 0; x < 500; ++x) {
            for (ui32 y = 0; y < 500; ++y) {
                TGeoObj newObj;
                newObj.Id = ++newId;
                newObj.Rect = TRect<TGeoCoord>(TGeoCoord(37 + x / 100.0, 55 + y / 100.0));
                hash.AddObject(newObj);
                objects.push_back(newObj);
            }
        }

        hash.GetPolicy()->SetCriticalObjectsCount(2);

        ui32 removedCount = 0;

        for (ui32 i = 0; i < objects.size();) {
            if ((i + removedCount) % 3 == 0) {
                hash.Remove(objects[i]);
                std::swap(objects[i], objects[objects.size() - 1 - removedCount]);
                ++removedCount;
            } else {
                ++i;
            }
        }

        objects.resize(objects.size() - removedCount);

        UNIT_ASSERT(CheckRectHash(objects, hash, geoRect));
    };

    namespace {

        class TRTreeBuilderAlg {
        public:
            TRTreeBuilderAlg(const TRect<TGeoCoord>& rect)
                : Rect(rect) {}

            struct TRectInfo {
                TRect<TGeoCoord> Rect;
                TVector<TGeoObj> Points;
            };

            TVector<TRectInfo> Execute(const TString& coords, double dist = 10, ui32 criticalObjectsCount = 10);

        private:
            TRect<TGeoCoord> Rect;
        };

        using TRectBuilder = TRectBuilderDefault<TGeoCoord, TGeoObj>;
        using TBuilderAlg = TRectHash<TGeoCoord, TGeoObj, TRectBuilder>;

        template <class TAlg>
        class TObjectsCollector {
            using TResult = TVector<TRTreeBuilderAlg::TRectInfo>;

        public:
            bool OnBigRect(const TRect<TGeoCoord>&, const typename TAlg::TNode&) { return true; }

            bool OnRect(const TRect<TGeoCoord>& rect, const TVector<TGeoObj>& points) {
                if (!points.empty()) {
                    Partition.push_back({ rect, points });
                }
                return false;
            }

            void OnObject(const TGeoObj&) {}

            const TResult& GetPartition() {
                return Partition;
            }

        private:
            TResult Partition;
        };

        TVector<TRTreeBuilderAlg::TRectInfo> TRTreeBuilderAlg::Execute(const TString& coords, double dist, ui32 criticalObjectsCount) {
            TBuilderAlg rectHash(Rect, TRectBuilder(), THashBuilderPolicyDefault<TGeoCoord, TGeoObj>(criticalObjectsCount, dist));

            TVector<TGeoCoord> points;
            CHECK_WITH_LOG(TGeoCoord::DeserializeVector(coords, points));

            INFO_LOG << "Start with objects=" << points.size() << ";critical_objects=" << criticalObjectsCount << ";rect=" << Rect.ToString() << Endl;

            for (ui32 i = 0; i < points.size(); ++i) {
                rectHash.AddObject(TGeoObj(i, points[i]));
            }

            TObjectsCollector<TBuilderAlg> collector;
            rectHash.Scan(collector);
            return collector.GetPartition();
        }
    }

    Y_UNIT_TEST(GeoSearchPolicy) {
        using THashType = TRectHash<TGeoCoord, TGeoObj, TRectBuilder, TGeoSearchHashPolicy<TGeoCoord, TGeoObj>>;

        TRect<TGeoCoord> rect(TGeoCoord(37.61696446328734, 55.738961905273044), TGeoCoord(37.62305844216917, 55.744626960995845));

        TGeoCoord p1(37.6189385691223, 55.742883954893294);
        TGeoCoord p2(37.621170167022704, 55.743077626099286);
        TGeoCoord p3(37.61902439981077, 55.740608245913485);
        TGeoCoord p4(37.62241471200559, 55.7408019284622);

        THashType hash(rect, TRectBuilder(), TGeoSearchHashPolicy<TGeoCoord, TGeoObj>(10, 250));
        {
            hash.AddObject(TGeoObj(1, p1));
            TObjectsCollector<THashType> collector;
            hash.Scan(collector);
            auto partition = collector.GetPartition();
            UNIT_ASSERT_VALUES_EQUAL(partition.size(), 1);
        }
        {
            hash.AddObject(TGeoObj(2, p2));
            TObjectsCollector<THashType> collector;
            hash.Scan(collector);
            auto partition = collector.GetPartition();
            UNIT_ASSERT_VALUES_EQUAL(partition.size(), 2);
        }
        {
            hash.AddObject(TGeoObj(3, p3));
            TObjectsCollector<THashType> collector;
            hash.Scan(collector);
            auto partition = collector.GetPartition();
            UNIT_ASSERT_VALUES_EQUAL(partition.size(), 3);
        }
        {
            hash.AddObject(TGeoObj(4, p4));
            TObjectsCollector<THashType> collector;
            hash.Scan(collector);
            auto partition = collector.GetPartition();
            UNIT_ASSERT_VALUES_EQUAL(partition.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(partition[0].Points.size(), 4);
        }

    }

    Y_UNIT_TEST(RTreeBuilder) {
        {
            TRect<TGeoCoord> rect(TGeoCoord(37.564382437759384, 55.74859684968339), TGeoCoord(37.58332956224058, 55.75455092213879));
            {
                TString points = "37.565262202316276 55.75401848127427 37.565305117660515 55.75324400880513 37.56700027375792 55.75361914583585 37.579574469619736 55.749940226243965 37.57959592729187 55.749310902467926 37.581312541061386 55.74963766800877";
                TRTreeBuilderAlg alg(rect);
                TVector<TRTreeBuilderAlg::TRectInfo> res = alg.Execute(points, 10, 10);
                UNIT_ASSERT_VALUES_EQUAL(res.size(), 1);
                UNIT_ASSERT_VALUES_EQUAL(res[0].Points.size(), 6);
            }
            {
                TString points = "37.565262202316276 55.75401848127427 37.565305117660515 55.75324400880513 37.56700027375792 55.75361914583585 37.579574469619736 55.749940226243965 37.57959592729187 55.749310902467926 37.581312541061386 55.74963766800877";
                TRTreeBuilderAlg alg(rect);
                TVector<TRTreeBuilderAlg::TRectInfo> res = alg.Execute(points, 10, 5);
                UNIT_ASSERT_VALUES_EQUAL(res.size(), 2);
                UNIT_ASSERT_VALUES_EQUAL(res[0].Points.size(), 3);
                UNIT_ASSERT_VALUES_EQUAL(res[1].Points.size(), 3);
            }
        }
        {
            TRect<TGeoCoord> rect(TGeoCoord(37.559060935073845, 55.74802196714191), TGeoCoord(37.588651064926125, 55.75512570806086));
            TString points = "37.58712757020569 55.74914751866677 37.585861567550644 55.74914751866677 37.585947398239114 55.74845766837489 37.58734214692687 55.748554490207496 37.5868057051239 55.748881262111 37.586097601943955 55.74894177512483 37.58659112840269 55.74945008071951 37.58689153581237 55.74860290103331 37.588114623123154 55.74868761983332 37.587985877090446 55.74918382623752 37.58759963899229 55.74974053807314 37.58661258607482 55.74861500373033";
            TRTreeBuilderAlg alg(rect);
            TVector<TRTreeBuilderAlg::TRectInfo> res = alg.Execute(points, 0, 10);
            UNIT_ASSERT_VALUES_EQUAL(res.size(), 2);
        }
        {
            TRect<TGeoCoord> rect(TGeoCoord(37.559060935073845, 55.74802196714191), TGeoCoord(37.588651064926125, 55.75512570806086));
            TString points = "37.58712757020569 55.74914751866677 37.585861567550644 55.74914751866677 37.585947398239114 55.74845766837489 37.58734214692687 55.748554490207496 37.5868057051239 55.748881262111 37.586097601943955 55.74894177512483 37.58659112840269 55.74945008071951 37.58689153581237 55.74860290103331 37.588114623123154 55.74868761983332 37.587985877090446 55.74918382623752 37.58759963899229 55.74974053807314 37.58661258607482 55.74861500373033";
            TRTreeBuilderAlg alg(rect);
            TVector<TRTreeBuilderAlg::TRectInfo> res = alg.Execute(points, 0, 2);

            UNIT_ASSERT(res.size() >= 6);
            for (ui32 i = 0; i < res.size(); ++i) {
                DEBUG_LOG << "check rect " << i << " " << res[i].Rect.ToString() << Endl;
                UNIT_ASSERT(res[i].Points.size() <= 3);
            }
        }
        {
            TRect<TGeoCoord> rect(TGeoCoord(37.55601658125963, 55.74076921304903), TGeoCoord(37.614467280112166, 55.76008421318099));
            TString points = "37.59856658365222 55.74189400583921 37.59860949899646 55.74305604820032 37.59745078470202 55.74288658585376";
            TRTreeBuilderAlg alg(rect);
            TVector<TRTreeBuilderAlg::TRectInfo> res = alg.Execute(points, 0, 2);

            UNIT_ASSERT(res.size() >= 2);
            for (ui32 i = 0; i < res.size(); ++i) {
                DEBUG_LOG << "check rect " << i << " " << res[i].Rect.ToString() << Endl;
                UNIT_ASSERT(res[i].Points.size() <= 2);
            }
        }
        {
            TRect<TGeoCoord> rect(TGeoCoord(-180, -90), TGeoCoord(180, 90));
            TString points = "37.59856658365222 55.74189400583921 37.59860949899646 55.74305604820032 37.59745078470202 55.74288658585376";
            TRTreeBuilderAlg alg(rect);
            TVector<TRTreeBuilderAlg::TRectInfo> res = alg.Execute(points, 50, 2);

            UNIT_ASSERT(res.size() >= 2);
            for (ui32 i = 0; i < res.size(); ++i) {
                DEBUG_LOG << "check rect " << i << " " << res[i].Rect.ToString() << Endl;
                UNIT_ASSERT(res[i].Points.size() <= 2);
            }
        }
        {
            TRect<TGeoCoord> rect(TGeoCoord(-180, -180), TGeoCoord(180, 180));
            TString points = "37.57517074830084 55.75677532267663 37.57804607636483 55.756000905179775 37.57435535676032 55.75643651692189 37.57753109223396 55.7558072983927 37.57808899170906 55.75721092573253 37.575771563120185 55.75633971473477 37.58242344147714 55.75638811585849 37.5859854150489 55.75723512575904 37.5864145684913 55.7563155141503 37.58834575898202 55.755202270962435 37.59036278016122 55.75711412547569 37.596242182321866 55.75796111954212 37.592723124094306 55.75629131355073 37.589933626718825 55.75677532267663 37.589075319834066 55.75788852077454 37.59040569550545 55.755589489603594 37.58765911347418 55.743632333463964 37.58714412934334 55.742131377796085 37.590305941306035 55.74253652319947 37.58893265029041 55.7430933302935 37.58755935927476 55.74130183552881 37.586271898947615 55.742584941524385 37.59116424819079 55.743698546345804 37.58961929579822 55.744400585156505 37.59369625350086 55.744812119251584 37.59249462386218 55.74282703224402 37.59206547041979 55.744812119251584 37.59459747572985 55.74403746321744 37.59807361861314 55.744352169094036 37.596271174155156 55.74319016549535 37.59425415297596 55.74181024108563 37.5917221476659 55.741689192746875 37.59489788313953 55.74314174792458";
            TRTreeBuilderAlg alg(rect);
            TVector<TRTreeBuilderAlg::TRectInfo> res = alg.Execute(points, 50, 5);

            UNIT_ASSERT(res.size() >= 13);
            for (ui32 i = 0; i < res.size(); ++i) {
                DEBUG_LOG << "check rect " << i << " " << res[i].Rect.ToString() << Endl;
                UNIT_ASSERT(res[i].Points.size() <= 5);
            }
        }
        {
            TRect<TGeoCoord> rect(TGeoCoord(-180, -90), TGeoCoord(180, 90));
            TString points = "37.57517074830084 55.75677532267663 37.57804607636483 55.756000905179775 37.57435535676032 55.75643651692189 37.57753109223396 55.7558072983927 37.57808899170906 55.75721092573253 37.575771563120185 55.75633971473477 37.58242344147714 55.75638811585849 37.5859854150489 55.75723512575904 37.5864145684913 55.7563155141503 37.58834575898202 55.755202270962435 37.59036278016122 55.75711412547569 37.596242182321866 55.75796111954212 37.592723124094306 55.75629131355073 37.589933626718825 55.75677532267663 37.589075319834066 55.75788852077454 37.59040569550545 55.755589489603594 37.58765911347418 55.743632333463964 37.58714412934334 55.742131377796085 37.590305941306035 55.74253652319947 37.58893265029041 55.7430933302935 37.58755935927476 55.74130183552881 37.586271898947615 55.742584941524385 37.59116424819079 55.743698546345804 37.58961929579822 55.744400585156505 37.59369625350086 55.744812119251584 37.59249462386218 55.74282703224402 37.59206547041979 55.744812119251584 37.59459747572985 55.74403746321744 37.59807361861314 55.744352169094036 37.596271174155156 55.74319016549535 37.59425415297596 55.74181024108563 37.5917221476659 55.741689192746875 37.59489788313953 55.74314174792458";
            TRTreeBuilderAlg alg(rect);
            TVector<TRTreeBuilderAlg::TRectInfo> res = alg.Execute(points, 50, 5);

            UNIT_ASSERT(res.size() >= 13);
            for (ui32 i = 0; i < res.size(); ++i) {
                DEBUG_LOG << "check rect " << i << " " << res[i].Rect.ToString() << Endl;
                UNIT_ASSERT(res[i].Points.size() <= 5);
            }
        }
    }
}
