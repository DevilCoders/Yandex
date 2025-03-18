#include <iostream>

#include <nginx/modules/strm_packager/src/fbs/description.fbs.h>
#include <nginx/modules/strm_packager/src/content/vod_description_details.h>
#include <nginx/modules/strm_packager/src/content/description.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/file.h>

Y_UNIT_TEST_SUITE(TVodDescriptionTest) {
    using namespace NStrm::NPackager;

    void TestParseVodDescription(const TString& testCatchUp, TVector<NDescription::TSourceInfo> expectedSegments, const ui64 expectedDuration) {
        flatbuffers::FlatBufferBuilder actualBuilder;

        NVodDescriptionDetails::ParseV2VodDescription(testCatchUp, actualBuilder);

        const auto& actualDescription = NFb::GetTDescription((const void*)actualBuilder.GetBufferPointer());
        const auto& actualVideoTrack = NDescription::GetVideoTrack(actualDescription, 1);

        UNIT_ASSERT_VALUES_EQUAL(actualVideoTrack->Duration(), expectedDuration);

        const auto& interval = TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(actualVideoTrack->Duration())};

        TVector<NDescription::TSourceInfo> actualSegments;
        NDescription::AddSourceInfos(actualVideoTrack->Segments(), interval, Ti64TimeMs(0), actualSegments);

        UNIT_ASSERT_VALUES_EQUAL(actualSegments.size(), expectedSegments.size());

        for (size_t i = 0; i < actualSegments.size(); ++i) {
            UNIT_ASSERT_VALUES_EQUAL(actualSegments[i].Path, expectedSegments[i].Path);
            UNIT_ASSERT_VALUES_EQUAL(actualSegments[i].Offset.MilliSeconds(), expectedSegments[i].Offset.MilliSeconds());
            UNIT_ASSERT_VALUES_EQUAL(actualSegments[i].Interval.Begin.MilliSeconds(), expectedSegments[i].Interval.Begin.MilliSeconds());
            UNIT_ASSERT_VALUES_EQUAL(actualSegments[i].Interval.End.MilliSeconds(), expectedSegments[i].Interval.End.MilliSeconds());
        }
    }

    Y_UNIT_TEST(ParseIntervals) {
        TString testCatchUp = TFileInput(SRC_("data/vod_description/vod_description_test.json")).ReadAll();

        TVector<NDescription::TSourceInfo> expectedSegments = {
            NDescription::TSourceInfo{
                "/test0_169_240p-t1-i0-s1650000000000-e1650000005000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(0),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t1-i1-s1650000005000-e1650000010000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(5000),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t1-i2-s1650000010000-e1650000015000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(10000),
            }};

        ui64 expectedDuration = 15000;

        TestParseVodDescription(testCatchUp, expectedSegments, expectedDuration);
    } // Y_UNIT_TEST(ParseIntervals)

    Y_UNIT_TEST(ParseMultipleStartEndIntervals) {
        TString testCatchUp = TFileInput(SRC_("data/vod_description/vod_description_multiple_start_end.json")).ReadAll();

        TVector<NDescription::TSourceInfo> expectedSegments = {
            NDescription::TSourceInfo{
                "/test0_169_240p-t1-i0-s1650000001234-e1650000005000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(3766)},
                Ti64TimeMs(1234),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t2-i1-s1650000005000-e1650000010000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(5000),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t2-i2-s1650000010000-e1650000015000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(10000),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t3-i3-s1650000015000-e1650000017567.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(2567)},
                Ti64TimeMs(15000),
            }};

        ui64 expectedDuration = 17567;

        TestParseVodDescription(testCatchUp, expectedSegments, expectedDuration);
    } // Y_UNIT_TEST(ParseMultipleStartEndIntervals)

    Y_UNIT_TEST(ParseIntervalsWithStubs) {
        TString testCatchUp = TFileInput(SRC_("data/vod_description/vod_description_with_stubs.json")).ReadAll();

        TVector<NDescription::TSourceInfo> expectedSegments = {
            NDescription::TSourceInfo{
                "/test0_169_240p-t1-i0-s1650000000000-e1650000005000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(0),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t1-i1-s1650000005000-e1650000010000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(5000),
            },
            NDescription::TSourceInfo{
                "/stub0_169_240p-5000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(10000),
            },
            NDescription::TSourceInfo{
                "/stub0_169_240p-5000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(15000),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t2-i4-s1650000020000-e1650000025000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(20000),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t2-i5-s1650000025000-e1650000030000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(25000),
            }};

        ui64 expectedDuration = 30000;

        TestParseVodDescription(testCatchUp, expectedSegments, expectedDuration);
    } // Y_UNIT_TEST(ParseIntervalsWithStubs)

    Y_UNIT_TEST(ParseIntervalsNonMultipleStubs) {
        TString testCatchUp = TFileInput(SRC_("data/vod_description/vod_description_non_multiple_stubs.json")).ReadAll();

        TVector<NDescription::TSourceInfo> expectedSegments = {
            NDescription::TSourceInfo{
                "/test0_169_240p-t1-i0-s1650000001234-e1650000005000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(3766)},
                Ti64TimeMs(1234),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t2-i1-s1650000005000-e1650000009321.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(4321)},
                Ti64TimeMs(5000),
            },
            NDescription::TSourceInfo{
                "/stub0_169_240p-5000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(679)},
                Ti64TimeMs(9321),
            },
            NDescription::TSourceInfo{
                "/stub0_169_240p-5000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(10000),
            },
            NDescription::TSourceInfo{
                "/stub0_169_240p-5000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(2222)},
                Ti64TimeMs(15000),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t3-i5-s1650000017222-e1650000020000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(2778)},
                Ti64TimeMs(17222),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t4-i6-s1650000020000-e1650000025000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(20000),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t5-i7-s1650000025000-e1650000029567.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(4567)},
                Ti64TimeMs(25000),
            }};

        ui64 expectedDuration = 29567;

        TestParseVodDescription(testCatchUp, expectedSegments, expectedDuration);
    } // Y_UNIT_TEST(ParseIntervalsNonMultipleStubs)

    Y_UNIT_TEST(ParseIntervalsNonMultipleStubsLong) {
        TString testCatchUp = TFileInput(SRC_("data/vod_description/vod_description_non_multiple_stubs_long.json")).ReadAll();

        TVector<NDescription::TSourceInfo> expectedSegments = {
            NDescription::TSourceInfo{
                "/test0_169_240p-t1-i0-s1650000001234-e1650000005000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(3766)},
                Ti64TimeMs(1234),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t2-i1-s1650000005000-e1650000009321.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(4321)},
                Ti64TimeMs(5000),
            },
            NDescription::TSourceInfo{
                "/stub0_169_240p-15000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(679)},
                Ti64TimeMs(9321),
            },
            NDescription::TSourceInfo{
                "/stub0_169_240p-15000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(15000)},
                Ti64TimeMs(10000),
            },
            NDescription::TSourceInfo{
                "/stub0_169_240p-15000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(15000)},
                Ti64TimeMs(25000),
            },
            NDescription::TSourceInfo{
                "/stub0_169_240p-15000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(7222)},
                Ti64TimeMs(40000),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t3-i6-s1650000047222-e1650000050000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(2778)},
                Ti64TimeMs(47222),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t4-i7-s1650000050000-e1650000055000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5000)},
                Ti64TimeMs(50000),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t5-i8-s1650000055000-e1650000059567.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(4567)},
                Ti64TimeMs(55000),
            }};

        ui64 expectedDuration = 59567;

        TestParseVodDescription(testCatchUp, expectedSegments, expectedDuration);
    } // Y_UNIT_TEST(ParseIntervalsNonMultipleStubsLong)

    Y_UNIT_TEST(ParseIntervalsShort) {
        TString testCatchUp = TFileInput(SRC_("data/vod_description/vod_description_test_short.json")).ReadAll();

        TVector<NDescription::TSourceInfo> expectedSegments = {
            NDescription::TSourceInfo{
                "/test0_169_240p-t1-i0-s1650000001234-e1650000004995.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(3761)},
                Ti64TimeMs(1234),
            }};

        ui64 expectedDuration = 4995;

        TestParseVodDescription(testCatchUp, expectedSegments, expectedDuration);
    } // Y_UNIT_TEST(ParseIntervalsShort)

    Y_UNIT_TEST(ParseIntervalsShortStub) {
        TString testCatchUp = TFileInput(SRC_("data/vod_description/vod_description_test_short_stub.json")).ReadAll();

        TVector<NDescription::TSourceInfo> expectedSegments = {
            NDescription::TSourceInfo{
                "/test0_169_240p-t1-i0-s1650000001234-e1650000004995.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(3761)},
                Ti64TimeMs(1234),
            },
            NDescription::TSourceInfo{
                "/stub0_169_240p-15000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(5)},
                Ti64TimeMs(4995),
            },
            NDescription::TSourceInfo{
                "/stub0_169_240p-15000.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(7)},
                Ti64TimeMs(5000),
            },
            NDescription::TSourceInfo{
                "/test0_169_240p-t2-i3-s1650000005007-e1650000005017.mp4",
                TIntervalMs{Ti64TimeMs(0), Ti64TimeMs(10)},
                Ti64TimeMs(5007),
            }};

        ui64 expectedDuration = 5017;

        TestParseVodDescription(testCatchUp, expectedSegments, expectedDuration);
    } // Y_UNIT_TEST(ParseIntervalsShortStub)

} // Y_UNIT_TEST_SUITE(TVodDescriptionTest)
