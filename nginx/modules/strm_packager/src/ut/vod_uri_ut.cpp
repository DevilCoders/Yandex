#include <random>

#include <nginx/modules/strm_packager/src/content/vod_uri.h>

#include <library/cpp/testing/unittest/registar.h>
#include <util/generic/vector.h>
#include <util/generic/queue.h>

using namespace NStrm;

Y_UNIT_TEST_SUITE(TVodUriTest) {
    Y_UNIT_TEST(Test) {
        using NStrm::NPackager::EContainer;
        using NStrm::NPackager::Ti64TimeMs;
        using NStrm::NPackager::TIntervalMs;
        using NStrm::NPackager::TVodUri;

        TMaybe<NStrm::NPackager::TVodUri> vu;

        vu.ConstructInPlace("/vod/zen-vod/vod-content/8291641797723515881/8390408-ce4a9d35-2db92d55-91e08f03/kaltura/desc_fa0eb5595f125fe0c6ee181032c30f3b/vT7Yjhw2qw1M/aid4/fragment-2681-a1.m4s");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>(4));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, false);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 2681);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::MP4);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetInterval(Ti64TimeMs(4000)), (TIntervalMs{Ti64TimeMs((2681 - 1) * 4000), Ti64TimeMs(2681 * 4000)}));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/zen-vod/vod-content/8291641797723515881/8390408-ce4a9d35-2db92d55-91e08f03/kaltura/desc_fa0eb5595f125fe0c6ee181032c30f3b.json");

        vu.ConstructInPlace("/vod/zen-vod/vod-content/2d9ce828220876393c452373d25b2b00/ed6fa548-855aeb06-8cd94842-a3364b5f/kaltura/desc_53e3fe826ed8eb36844aaaf6342f164a/vUw_UYLuM9CE/vid123aid456/seg-2-f1-v1-f2-a1.ts");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>(123));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>(456));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, false);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 2);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::TS);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetInterval(Ti64TimeMs(4000)), (TIntervalMs{Ti64TimeMs((2 - 1) * 4000), Ti64TimeMs(2 * 4000)}));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/zen-vod/vod-content/2d9ce828220876393c452373d25b2b00/ed6fa548-855aeb06-8cd94842-a3364b5f/kaltura/desc_53e3fe826ed8eb36844aaaf6342f164a.json");

        vu.ConstructInPlace("/vod/zen-vod/vod-content/2d9ce828220876393c452373d25b2b00/ed6fa548-855aeb06-8cd94842-a3364b5f/kaltura/desc_53e3fe826ed8eb36844aaaf6342f164a/vUw_UYLuM9CE/aid456vid123/seg-2-f1-v1-f2-a1.ts");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>(123));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>(456));

        vu.ConstructInPlace("/vod/vh-music-converted/content/10146730489165578276/9326dc31-b089e500-cb8bf7b8-fa3f5409/kaltura/desc_7904df4b75cdb9331e669a20e7897583/4bd09c579f72d8109edefda0650aca8a/aid1/seg-13-f1-a1.ts");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>(1));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, false);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 13);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::TS);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetInterval(Ti64TimeMs(4000)), (TIntervalMs{Ti64TimeMs((13 - 1) * 4000), Ti64TimeMs(13 * 4000)}));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/vh-music-converted/content/10146730489165578276/9326dc31-b089e500-cb8bf7b8-fa3f5409/kaltura/desc_7904df4b75cdb9331e669a20e7897583.json");

        vu.ConstructInPlace("/vod/vh-canvas-converted/vod-content/4484321809083885431/23f704a2-8e478bf7-bb47efaf-5cae8ee9/kaltura/desc_47a9d510d65f7f442108e52999dc6afd/6096042699782404499/aid1/fragment-2-a1.m4s");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>(1));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, false);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 2);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::MP4);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetInterval(Ti64TimeMs(4000)), (TIntervalMs{Ti64TimeMs((2 - 1) * 4000), Ti64TimeMs(2 * 4000)}));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/vh-canvas-converted/vod-content/4484321809083885431/23f704a2-8e478bf7-bb47efaf-5cae8ee9/kaltura/desc_47a9d510d65f7f442108e52999dc6afd.json");

        vu.ConstructInPlace("/vod/vh-ottenc-converted/vod-content/07a7c10354e145d895d0adf818a84165/9401751x1651848176x7c327643-b621-4c2d-9c64-4eedd1d3d064/kaltura/dash_drm_sdr_hd_avc_aac_67c3e2198f1d4547e396e04c8a77a900d3102b488167b8a6684c2b16788a8573/07a7c10354e145d895d0adf818a84165/aid0/fragment-509-a1.m4s");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>(0));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, false);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 509);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::MP4);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetInterval(Ti64TimeMs(4000)), (TIntervalMs{Ti64TimeMs((509 - 1) * 4000), Ti64TimeMs(509 * 4000)}));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/vh-ottenc-converted/vod-content/07a7c10354e145d895d0adf818a84165/9401751x1651848176x7c327643-b621-4c2d-9c64-4eedd1d3d064/kaltura/dash_drm_sdr_hd_avc_aac_67c3e2198f1d4547e396e04c8a77a900d3102b488167b8a6684c2b16788a8573.json");

        vu.ConstructInPlace("/vod/vh-ottenc-converted/vod-content/2688f2b6971048f682923982f11be2c0/9265323x1648558506xa7b9f165-f21a-4877-945c-caf2518208f7/kaltura/dash_drm_sdr_hd_avc_aac_6512e870b9ed2b21d1aabbf98dc24616719bea83db700b57de2536021dd5a9c8/2688f2b6971048f682923982f11be2c0/vid1/fragment-7-v1.m4s");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>(1));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, false);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 7);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::MP4);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetInterval(Ti64TimeMs(4000)), (TIntervalMs{Ti64TimeMs((7 - 1) * 4000), Ti64TimeMs(7 * 4000)}));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/vh-ottenc-converted/vod-content/2688f2b6971048f682923982f11be2c0/9265323x1648558506xa7b9f165-f21a-4877-945c-caf2518208f7/kaltura/dash_drm_sdr_hd_avc_aac_6512e870b9ed2b21d1aabbf98dc24616719bea83db700b57de2536021dd5a9c8.json");

        vu.ConstructInPlace("/vod/vh-ottenc-converted/vod-content/07e48437c2d74f218dc101160f43f4a8/9401751x1652365788x01dbb05c-73a7-41ad-a317-3e7711eb03bd/kaltura/hls_drm_sdr_hd_avc_aac_74033ca5c8d20ee34f7a4c820fe5fc5f15c566ed3a5f89ed2a0b0a7baf19602a/07e48437c2d74f218dc101160f43f4a8/aid0/seg-422-f1-a1.ts");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>(0));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, false);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 422);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::TS);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetInterval(Ti64TimeMs(4000)), (TIntervalMs{Ti64TimeMs((422 - 1) * 4000), Ti64TimeMs(422 * 4000)}));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/vh-ottenc-converted/vod-content/07e48437c2d74f218dc101160f43f4a8/9401751x1652365788x01dbb05c-73a7-41ad-a317-3e7711eb03bd/kaltura/hls_drm_sdr_hd_avc_aac_74033ca5c8d20ee34f7a4c820fe5fc5f15c566ed3a5f89ed2a0b0a7baf19602a.json");

        vu.ConstructInPlace("/vod/vh-ottenc-converted/vod-content/4069eb4188110a819fa1b77a6d726963/9287010x1649079785xa2924cc2-d3db-4f8b-9a2b-81d0a6458c6a/kaltura/hls_drm_sdr_hd_avc_aac_b660311038fc49f07a32864caf64a272e047c08ecf02522183103b482b3e1f8f/4069eb4188110a819fa1b77a6d726963/vid1/seg-606-f1-v1.ts");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>(1));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, false);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 606);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::TS);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetInterval(Ti64TimeMs(4000)), (TIntervalMs{Ti64TimeMs((606 - 1) * 4000), Ti64TimeMs(606 * 4000)}));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/vh-ottenc-converted/vod-content/4069eb4188110a819fa1b77a6d726963/9287010x1649079785xa2924cc2-d3db-4f8b-9a2b-81d0a6458c6a/kaltura/hls_drm_sdr_hd_avc_aac_b660311038fc49f07a32864caf64a272e047c08ecf02522183103b482b3e1f8f.json");

        vu.ConstructInPlace("/vod/vh-ottenc-converted/vod-content/4687d0252e341e589beed055fb46deed/9287010x1649376225x09e1d72f-c26e-4e37-9029-8b3287695161/kaltura/dash_drm_sdr_hd_avc_ec3_072a4573d03db92221fd6ae4b7a11dab30bab7f43b75d7d327bca70fd00ed0f2/4687d0252e341e589beed055fb46deed/aid0/init-a1.mp4");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>(0));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, true);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 0);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::MP4);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/vh-ottenc-converted/vod-content/4687d0252e341e589beed055fb46deed/9287010x1649376225x09e1d72f-c26e-4e37-9029-8b3287695161/kaltura/dash_drm_sdr_hd_avc_ec3_072a4573d03db92221fd6ae4b7a11dab30bab7f43b75d7d327bca70fd00ed0f2.json");

        vu.ConstructInPlace("/vod/vh-ottenc-converted/vod-content/4687d0252e341e589beed055fb46deed/9287010x1649376225x09e1d72f-c26e-4e37-9029-8b3287695161/kaltura/dash_drm_sdr_hd_avc_ec3_072a4573d03db92221fd6ae4b7a11dab30bab7f43b75d7d327bca70fd00ed0f2/4687d0252e341e589beed055fb46deed/vid4/init-v1.mp4");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>(4));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, true);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 0);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::MP4);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/vh-ottenc-converted/vod-content/4687d0252e341e589beed055fb46deed/9287010x1649376225x09e1d72f-c26e-4e37-9029-8b3287695161/kaltura/dash_drm_sdr_hd_avc_ec3_072a4573d03db92221fd6ae4b7a11dab30bab7f43b75d7d327bca70fd00ed0f2.json");

        vu.ConstructInPlace("/vod/vh-zen-converted/vod-content/1352837541748586994/live/kaltura/v2/vA_HQM_G9cmo/vid4/init-v1.mp4");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>(4));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, true);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 0);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::MP4);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/vh-zen-converted/vod-content/1352837541748586994/live/kaltura/v2.json");

        vu.ConstructInPlace("/vod/vh-zen-converted/vod-content/559154906554743304/live/kaltura/v2/2271082787602523705/aid1/fragment-695-a1.m4s");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>(1));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, false);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 695);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::MP4);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetInterval(Ti64TimeMs(4000)), (TIntervalMs{Ti64TimeMs((695 - 1) * 4000), Ti64TimeMs(695 * 4000)}));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/vh-zen-converted/vod-content/559154906554743304/live/kaltura/v2.json");

        vu.ConstructInPlace("/vod/vh-zen-converted/vod-content/7184490432709027451/live/kaltura/v2/5403439497360418419/vid3/fragment-2060-v1.m4s");
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().VideoTrackId, TMaybe<ui32>(3));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetTrackInfo().AudioTrackId, TMaybe<ui32>());
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().IsInitSegment, false);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Number, 2060);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetChunkInfo().Container, EContainer::MP4);
        UNIT_ASSERT_VALUES_EQUAL(vu->GetInterval(Ti64TimeMs(4000)), (TIntervalMs{Ti64TimeMs((2060 - 1) * 4000), Ti64TimeMs(2060 * 4000)}));
        UNIT_ASSERT_VALUES_EQUAL(vu->GetDescriptionUri(), "/vh-zen-converted/vod-content/7184490432709027451/live/kaltura/v2.json");
    } // Y_UNIT_TEST(Test)
} // Y_UNIT_TEST_SUITE(TVodUriTest)
