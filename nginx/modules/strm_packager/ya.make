LIBRARY()

OWNER(
    dronimal
    g:contrib
    g:strm-admin
)

BUILD_ONLY_IF(LINUX)

NO_UTIL()

PEERDIR(
    contrib/libs/protobuf
    contrib/nginx/core/src/http
    infra/yp_service_discovery/api
    library/cpp/threading/future
    nginx/modules/strm_packager/src/proto
    strm/media/formats/mpegts
    strm/media/transcoder/mp4muxer
    strm/plgo/pkg/proto/vod/description/v1
    strm/plgo/pkg/proto/vod/description/v2
    strm/trns_manager/proto/api/liveinfo
    strm/trns_manager/proto/api/stream
)

ADDINCL(
    contrib/libs/grpc
)

SRCS(
    src/base/config.cpp
    src/base/context.cpp
    src/base/grpc_resolver.cpp
    src/base/headers.cpp
    src/base/live_manager.cpp
    src/base/shm_cache.cpp
    src/base/timer.cpp
    src/base/workers.cpp
    src/common/avcC_box_util.cpp
    src/common/cenc_cipher.cpp
    src/common/convert_subtitles_raw_to_ttml.cpp
    src/common/dispatcher.cpp
    src/common/drm_info_util.cpp
    src/common/encrypting_buffer.cpp
    src/common/evp_cipher.cpp
    src/common/fragment_cutter.cpp
    src/common/h2645_util_avc.cpp
    src/common/h2645_util_hevc.cpp
    src/common/hmac.cpp
    src/common/hvcC_box_util.cpp
    src/common/mp4_common.cpp
    src/common/muxer.cpp
    src/common/muxer_caf.cpp
    src/common/muxer_mp4.cpp
    src/common/muxer_mpegts.cpp
    src/common/muxer_webvtt.cpp
    src/common/order_manager.cpp
    src/common/sender.cpp
    src/common/source.cpp
    src/common/source_concat.cpp
    src/common/source_file.cpp
    src/common/source_live.cpp
    src/common/source_moov_data.cpp
    src/common/source_mp4_file.cpp
    src/common/source_tracks_select.cpp
    src/common/source_union.cpp
    src/common/source_webvtt_file.cpp
    src/common/timed_meta.cpp
    src/common/track_data.cpp
    src/content/common_uri.cpp
    src/content/description.cpp
    src/content/live_uri.cpp
    src/content/music_uri.cpp
    src/content/vod_description.cpp
    src/content/vod_description_details.cpp
    src/content/vod_prerolls.cpp
    src/content/vod_uri.cpp
    src/fbs/description.fbs
    src/temp/test_cache_worker.cpp
    src/temp/test_live.cpp
    src/temp/test_live_manager_subscribe_worker.cpp
    src/temp/test_mp4_vod.cpp
    src/temp/test_mpegts_vod.cpp
    src/temp/test_read_worker.cpp
    src/temp/test_timed_meta.cpp
    src/temp/test_worker.cpp
    src/workers/kaltura_imitation_worker.cpp
    src/workers/live_worker.cpp
    src/workers/music_worker.cpp
    src/workers/vod_worker.cpp
)

GENERATE_ENUM_SERIALIZATION(src/common/muxer.h)
GENERATE_ENUM_SERIALIZATION(src/common/muxer_mp4.h)
GENERATE_ENUM_SERIALIZATION(src/common/track_data.h)
GENERATE_ENUM_SERIALIZATION(strm/plgo/pkg/proto/vod/description/v2/description.pb.h)

RESOURCE(
    strm/common/clips/clip_prefix_map.json clip_prefix_map.json
)

END()

RECURSE(
    src/ut
    src/proto
)
