#include <nginx/modules/strm_packager/src/common/source_moov_data.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/memory/blob.h>

using namespace NStrm::NMP4Muxer;
using namespace NStrm::NPackager;

Y_UNIT_TEST_SUITE(TSourceMoovCacheDataTest) {
    Y_UNIT_TEST(TestFastAndSlowEqual) {
        struct TTestFile {
            TString File;
            TVector<size_t> TracksSizes;
        };

        const TTestFile files[] = {
            {"video_1_3e338618de2d7b089a52943675de4890.mp4", {2268}},
            {"video_1_7931f6e5b751ab3a9d76f6003fb25e27.mp4", {3581}},
            {"source.mp4", {750, 1406}},
            {"sample.mp4", {750, 1252}},
        };

        for (int kalturaMode = 0; kalturaMode < 2; ++kalturaMode) {
            for (const TTestFile& tf : files) {
                const TBlob file = TBlob::FromFileSingleThreaded(tf.File);

                TMaybe<TDummyBox<'moov'>> moov;

                TBlobReader reader((ui8 const*)file.Data(), file.Length());
                ReadTBoxContainer(reader, file.Length(), moov);

                UNIT_ASSERT(moov.Defined());

                TBufferWriter writer;
                moov->WorkIO(writer);

                TBuffer slow = SaveTFileMediaDataSlow(TArrayRef<ui8>((ui8*)writer.Buffer().Data(), writer.Buffer().Size()), kalturaMode);
                TBuffer fast = SaveTFileMediaDataFast(TArrayRef<ui8>((ui8*)writer.Buffer().Data(), writer.Buffer().Size()), kalturaMode);

                UNIT_ASSERT(slow == fast);

                const TFileMediaData md1 = LoadTFileMediaDataFromMoovData(TIntervalP{}, fast.Data(), fast.Size());
                const TFileMediaData md2 = LoadTFileMediaDataFromMoovData(TIntervalP{Ms2P(-1000), Ms2P(1e12)}, fast.Data(), fast.Size());

                UNIT_ASSERT_VALUES_EQUAL(tf.TracksSizes.size(), md1.TracksSamples.size());
                UNIT_ASSERT_VALUES_EQUAL(tf.TracksSizes.size(), md2.TracksSamples.size());

                for (size_t ti = 0; ti < tf.TracksSizes.size(); ++ti) {
                    UNIT_ASSERT_VALUES_EQUAL(md1.TracksSamples[ti].size(), 0);
                    UNIT_ASSERT_VALUES_EQUAL(md2.TracksSamples[ti].size(), tf.TracksSizes[ti]);
                }
            }
        }
    } // Y_UNIT_TEST(TestFastAndSlowEqual)
} // Y_UNIT_TEST_SUITE(TSourceMoovCacheDataTest)
