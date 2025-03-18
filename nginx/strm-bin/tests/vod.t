#!/usr/bin/perl

use warnings;
use strict;

use Test::More;

BEGIN { use FindBin; chdir($FindBin::Bin); }

use lib 'lib';
use Test::Nginx qw/ :DEFAULT http_content /;
use Digest::MD5 qw/ md5_hex /;


select STDERR; $| = 1;
select STDOUT; $| = 1;

my $t = Test::Nginx->new()->plan(1550);

$t->write_file_expand('nginx.conf', <<'EOF');

%%TEST_GLOBALS%%

daemon off;

events {
}

http {
    %%TEST_GLOBALS_HTTP%%

    geo $dollar {
        default "$";
    }

    packager_test_shm_zone moov_cache_zone1 300M 1000000 1100 500;

    server {
        listen       127.0.0.1:8080;
        server_name  localhost;

        location /freeze {
            echo_sleep 1111111;
            echo 123;
        }

        location /echo {
            echo 123;
        }

        location /proxy/ {
            rewrite ^/proxy/(.*)$ /$1 break;
            proxy_pass http://127.0.0.1:8080;
            proxy_buffering off;
        }

        location /drm_keys_no_iv {
            echo '[{"key":"RFE+yyEhCe4HSbBEaEwgcg==","key_id":"3API1HHvLg7X2ITIspDL1g=="}]';
        }

        location /drm_keys {
            echo '[{"key":"RFE+yyEhCe4HSbBEaEwgcg==","key_id":"3API1HHvLg7X2ITIspDL1g==","iv":"9LoKGCddaaAb1WRN0adJ/Q=="}]';
        }

        location /kalvod/ {
            vod_performance_counters vod_vod_perf;
            vod_mode mapped;
            vod_media_set_map_uri /playlist/$uri;

            vod_subrequest_variables_mode copy;

            vod_max_mapping_response_size 256k;
            vod_cache_buffer_size 1m;

            vod_upstream_location /proxy/playlistjson;

            vod_remote_upstream_location /proxy;

            vod_segment_duration 5000;

            location /kalvod/dash {
                vod dash;
            }

            location /kalvod/hls {
                vod hls;
            }

            location /kalvod/mss {
                vod mss;
            }

            location /kalvod/mssdrm {
                vod mss;

                vod_drm_upstream_location "/proxy/drm_keys";
                vod_drm_enabled 1;
            }

            location /kalvod/dashdrm {
                vod dash;

                vod_drm_upstream_location "/proxy/drm_keys";
                vod_drm_enabled 1;
                vod_drm_request_uri $uri;
                vod_drm_clear_lead_segment_count 0;
            }

            location /kalvod/hlsdrm {
                vod hls;

                vod_hls_encryption_method sample-aes;

                vod_drm_upstream_location "/proxy/drm_keys";
                vod_drm_enabled 1;
                vod_drm_request_uri $uri;
                vod_drm_clear_lead_segment_count 0;
            }
        }

        location /pakkalim/ {
            packager_kaltura_imitation_worker;

            packager_max_media_data_subrequest_size 1M;
            packager_moov_scan_block_size 4K;

            packager_playlist_json_uri /proxy/playlistjson/playlist$uri;
            packager_playlist_json_args $args&orig_uri=$uri&for-regional-cache=1;

            packager_content_location /proxy;

            packager_subtitles_ttml_style 'tts:textAlign="center" tts:backgroundColor="#00000050" tts:color="#ffffff"';
            packager_subtitles_ttml_region 'tts:origin="0% 0%" tts:extent="100% 100%" tts:overflow="visible" tts:displayAlign="after"';

        }

        location /pakvod/ {
            packager_moov_shm_cache_zone moov_cache_zone1;

            packager_max_media_data_subrequest_size 1M;
            packager_moov_scan_block_size 4K;
            packager_chunk_duration_milliseconds 5000;

            packager_subrequest_headers_drop accept-charset;
            packager_subrequest_headers_drop accept-datetime;
            packager_subrequest_headers_drop accept-encoding;
            packager_subrequest_headers_drop accept-language;
            packager_subrequest_headers_drop if-match;
            packager_subrequest_headers_drop if-modified-since;
            packager_subrequest_headers_drop if-none-match;
            packager_subrequest_headers_drop if-range;
            packager_subrequest_headers_drop if-unmodified-since;


            packager_meta_location /proxy/json_meta;
            packager_content_location /proxy;

            location /pakvod/intervals/ {
                packager_vod;
                packager_meta_location /proxy/json_meta_intervals;
            }

            location /pakvod/nodrm/ {
                packager_vod;
                packager_allow_drm_content_unencrypted 1;
            }

            location /pakvod/drmcenc/ {
                packager_vod;
                packager_drm_enable 1;
                packager_drm_request_uri /proxy/drm_keys_no_iv;

                packager_drm_mp4_protection_scheme CENC;
            }

            location /pakvod/drmcbcs/ {
                packager_vod;
                packager_drm_enable 1;
                packager_drm_request_uri /proxy/drm_keys;

                packager_drm_mp4_protection_scheme CBCS;
            }
        }

        location /json_meta {
            echo '
            {
                "version": 2,
                "video_rendition_sets": [
                    {
                        "width": 426,
                        "height": 240,
                        "framerate": 25.0,
                        "label": "240p",
                        "videos": [
                            {
                                "segments": [
                                    { "path": "/test_ctc0_169_240p-1652874610000.mp4", "duration": 10.000 },
                                    { "path": "/test_ctc0_169_240p-1652874620000.mp4", "duration": 10.000 },
                                    { "path": "/test_ctc0_169_240p-1652874630000.mp4", "duration": 10.000 },
                                    { "path": "/test_ctc0_169_240p-1652874640000.mp4", "duration": 10.000 }
                                ],
                                "id": 0
                            }
                        ]
                    }
                ],
                "audio_rendition_sets": [
                    {
                        "language": "rus",
                        "label": "Русский",
                        "audios": [
                            {
                                "segments": [
                                    { "path": "/test_ctc0_audio1_rus-1652874610000.mp4", "duration": 10.000 },
                                    { "path": "/test_ctc0_audio1_rus-1652874620000.mp4", "duration": 10.000 },
                                    { "path": "/test_ctc0_audio1_rus-1652874630000.mp4", "duration": 10.000 },
                                    { "path": "/test_ctc0_audio1_rus-1652874640000.mp4", "duration": 10.000 }
                                ],
                                "id": 0
                            }
                        ]
                    }
                ],
                "timescale": 1000,
                "segment_duration": 5000,
                "drm": {
                    "content_id": "123"
                }
            }';
        }

        location /json_meta_intervals {
            echo '
            {
                "version": 2,
                "video_rendition_sets": [
                    {
                        "width": 426,
                        "height": 240,
                        "framerate": 25.0,
                        "label": "240p",
                        "videos": [
                            {
                                "segment_intervals": [
                                    {
                                        "templates": [
                                            {
                                                "path_template":  "/test_ctc0_169_240p-${dollar}{end_time}.mp4",
                                                "segment_duration":  10000,
                                                "count": 4
                                            }
                                        ],
                                        "start_time":  1652874600000,
                                        "end_time":  1652874640000
                                    }
                                ],
                                "stub":  {
                                	"path":  "/test_ctc0_169_240p-10000.mp4",
                                    "duration": 10000
                                },
                                "id": 0
                            }
                        ]
                    }
                ],
                "audio_rendition_sets": [
                    {
                        "language": "rus",
                        "label": "Русский",
                        "audios": [
                            {
                                "segment_intervals": [
                                    {
                                        "templates": [
                                            {
                                                "path_template":  "/test_ctc0_audio1_rus-${dollar}{end_time}.mp4",
                                                "segment_duration":  10000,
                                                "count":  4
                                            }
                                        ],
                                        "start_time":  1652874600000,
                                        "end_time":  1652874640000
                                    }
                                ],
                                "stub":  {
                                    "path":  "/test_ctc0_audio1_rus-10000.mp4",
                                    "duration": 10000
                                },
                                "id": 0
                            }
                        ]
                    }
                ],
                "timescale": 1000,
                "segment_duration": 5000
            }';
        }

        # files 1652874650000 do not exist, but it is "needed" for kaltura "look-ahead" logic in mss (dont know why or what it is) to get last two existing segments
        location /playlistjson/playlist/ {
            set $video_seq '{ "clips": [
                        { "path": "/test_ctc0_169_240p-1652874610000.mp4", "type": "source" },
                        { "path": "/test_ctc0_169_240p-1652874620000.mp4", "type": "source" },
                        { "path": "/test_ctc0_169_240p-1652874630000.mp4", "type": "source" },
                        { "path": "/test_ctc0_169_240p-1652874640000.mp4", "type": "source" },
                        { "path": "/test_ctc0_169_240p-1652874650000.mp4", "type": "source" }
                    ] }';

            set $audio_seq '{ "clips": [
                        { "path": "/test_ctc0_audio1_rus-1652874610000.mp4", "type": "source" },
                        { "path": "/test_ctc0_audio1_rus-1652874620000.mp4", "type": "source" },
                        { "path": "/test_ctc0_audio1_rus-1652874630000.mp4", "type": "source" },
                        { "path": "/test_ctc0_audio1_rus-1652874640000.mp4", "type": "source" },
                        { "path": "/test_ctc0_audio1_rus-1652874650000.mp4", "type": "source" }
                    ] }';

            set $subtitle_seq '{ "clips": [
                        { "path": "/test_ctc0_subtitle1_rus-1652874610000.vtt", "type": "source" },
                        { "path": "/test_ctc0_subtitle1_rus-1652874620000.vtt", "type": "source" },
                        { "path": "/test_ctc0_subtitle1_rus-1652874630000.vtt", "type": "source" },
                        { "path": "/test_ctc0_subtitle1_rus-1652874640000.vtt", "type": "source" },
                        { "path": "/test_ctc0_subtitle1_rus-1652874650000.vtt", "type": "source" }
                    ] }';

            set $seqs '';

            if ($uri ~ '^.*(-v1|video=).*$') {
                set $seqs '$seqs,$video_seq';
            }

            if ($uri ~ '^.*(-a1|audio=).*$') {
                set $seqs '$seqs,$audio_seq';
            }

            if ($uri ~ '^.*-s1.*$') {
                set $seqs '$seqs,$subtitle_seq';
            }

            if ($seqs ~ ^,((\n|.)*)$) {
                set $seqs $1;
            }

            echo '
            {
                "sequences": [
                    $seqs
                ],
                "durations": [
                    10000,
                    10000,
                    10000,
                    10000,
                    10000
                ],
                "discontinuity": false,
                "firstClipTime": 1652874600000,
                "segmentBaseTime": 0,
                "segmentDuration": 5000,
                "playlistType": "live"
            }';
        }


        location ~ ^/playlistjson/playlist/.*hevc_ec3.*$ {
            echo '
            {
                "sequences": [
                    { "clips": [
                        { "path": "/hevc_ec3.mp4", "type": "source" }
                    ] }
                ],
                "durations": [
                    10000
                ],
                "discontinuity": false,
                "firstClipTime": 0,
                "segmentBaseTime": 0,
                "segmentDuration": 5000,
                "playlistType": "live"
            }';
        }



        location ~ ^/json_meta.*hevc_ec3.*$ {
            echo '
            {
                "version": 2,
                "video_rendition_sets": [
                    {
                        "width": 1920,
                        "height": 1080,
                        "framerate": 59.94,
                        "label": "hevc_ec3",
                        "videos": [
                            {
                                "segments": [
                                    { "path": "/hevc_ec3.mp4", "duration": 10.000 }
                                ],
                                "id": 0
                            }
                        ]
                    }
                ],
                "audio_rendition_sets": [
                    {
                        "language": "rus",
                        "label": "Русский",
                        "audios": [
                            {
                                "segments": [
                                    { "path": "/hevc_ec3.mp4", "duration": 10.000 }
                                ],
                                "id": 0
                            }
                        ]
                    }
                ],
                "timescale": 1000,
                "segment_duration": 5000,
                "drm": {
                    "content_id": "123"
                }
            }';
        }

        location / {
            # root /home/dronimal/tmp/ngx_data/test/;
            root %%TEST_DATA_PATH%%/;
        }

    }
}

EOF

$t->run();

my $testdir = $t->testdir();
my $ffprobe_bin = $ENV{TEST_FFPROBE_BINARY};
my $ffmpeg_bin = $ENV{TEST_FFMPEG_BINARY};


my $t1;

$t1 = http_get('/echo');
like($t1, qr/ 200 /, '/echo status - 200');
like($t1, qr/^123$/m, '/echo correct content');

$t1 = http_get('/proxy/echo');
like($t1, qr/ 200 /, '/proxy/echo status - 200');
like($t1, qr/^123$/m, '/proxy/echo correct content');
is(http_content($t1), "123\n", '/proxy/echo exact content');
is(md5_hex(http_content($t1)), 'ba1f2511fc30423bdbb183fe33f3dd0f', '/proxy/echo md5 content');
$t->write_file('t123', http_content($t1));



my $empty_md5 = 'd41d8cd98f00b204e9800998ecf8427e';

# while md5 of whole file can change in case of some modifications in packager/kaltura, the video_md5 and audio_md5 should not, otherwise it would mean that wrong samples were selected
sub check_file($$$$$$$$) {
    my ($uri_prefix, $args, $file_prefix, $name, $init, $md5, $video_md5, $audio_md5) = @_;
    my $tt = http_get("$uri_prefix/$name?$args");
    like(substr($tt, 0, 100), qr/ 200 /, "$file_prefix.$name status - 200");
    is(md5_hex(http_content($tt)), $md5, "$file_prefix.$name  md5 content");
    $t->write_file("$file_prefix.$name", http_content($tt));

    if ($video_md5 eq $empty_md5 && $audio_md5 eq $empty_md5) {
        return;
    }

    my $tmpinput = "$testdir/$file_prefix.$name";

    if ($init ne '') {
        is(system("bash", "-c", "cat '$testdir/$file_prefix.$init' '$testdir/$file_prefix.$name' > '$testdir/$file_prefix.$name.tmp.bin'"), 0, "cat call $file_prefix.$name");
        $tmpinput = "$testdir/$file_prefix.$name.tmp.bin";
    }

    # my $tmpinput2 = "$testdir/$file_prefix.$name.tmp.ts";
    # my $tmpinput3 = "$testdir/$file_prefix.$name.tmp.mp4";

    # is(system("bash", "-c", "$ffmpeg_bin -nostdin -hide_banner -i '$tmpinput' -c copy '$tmpinput2' &> /dev/null"), 0, "ffmpeg call  $tmpinput2");
    # is(system("bash", "-c", "$ffmpeg_bin -nostdin -hide_banner -i '$tmpinput2' -c copy '$tmpinput3' &> /dev/null"), 0, "ffmpeg call  $tmpinput3");

    my $tmpinput2 = $tmpinput;
    my $tmpinput3 = $tmpinput;

    if ($file_prefix !~ /-drm/ ) {
        $tmpinput2 = "$testdir/$file_prefix.$name.tmp.ts";
        $tmpinput3 = "$testdir/$file_prefix.$name.tmp.mp4";

        is(system("bash", "-c", "$ffmpeg_bin -nostdin -hide_banner -i '$tmpinput' -c copy '$tmpinput2' &> /dev/null"), 0, "ffmpeg call  $tmpinput2");
        is(system("bash", "-c", "$ffmpeg_bin -nostdin -hide_banner -i '$tmpinput2' -c copy '$tmpinput3' &> /dev/null"), 0, "ffmpeg call  $tmpinput3");
    }

    is(system("bash", "-c", "$ffprobe_bin -hide_banner -show_data -show_entries packet=flags,data -select_streams v:0 '$tmpinput3' > '$testdir/$file_prefix.$name.video_packets' 2> /dev/null"), 0, "ffprobe call  $file_prefix.$name.video_packets");
    is(system("bash", "-c", "$ffprobe_bin -hide_banner -show_data -show_entries packet=flags,data -select_streams a:0 '$tmpinput3' > '$testdir/$file_prefix.$name.audio_packets' 2> /dev/null"), 0, "ffprobe call  $file_prefix.$name.audio_packets");






    my $video = $t->read_file("$file_prefix.$name.video_packets");
    my $audio = $t->read_file("$file_prefix.$name.audio_packets");

    is(md5_hex($video), $video_md5, "$file_prefix.$name video md5 content");
    is(md5_hex($audio), $audio_md5, "$file_prefix.$name audio md5 content");
}




my $s1_a1_md5 = '41dc4ac2d88507a0eccd81b0a8b9c083';
my $s2_a1_md5 = '700a82de566fa05b43da6f70576159c5';
my $s3_a1_md5 = 'c58fa30c34e524d4f8b05a50c350b126';
my $s4_a1_md5 = '177e2666e3857e463542d67a09b5ea28';
my $s5_a1_md5 = 'cd903666d58253e511ff2256aef784d9';
my $s6_a1_md5 = 'de9dac444d42fbd8bd0c1ea4708177a1';
my $s7_a1_md5 = '622d39e608def04019317bd93984c9c8';
my $s8_a1_md5 = '18e102eba4a75d0031fd3730f8ede1ba';

my $s1_v1_md5 = '313b869855a10289e0fee02e14b13728';
my $s2_v1_md5 = '916113abe7b9990f4f1a5310b7028aba';
my $s3_v1_md5 = 'b82f6f860317c70dc60a3009fcbca455';
my $s4_v1_md5 = '87b36cd7f5decdbf58ce3aa9b0692052';
my $s5_v1_md5 = '8cca6e252931078a03166bae63b0c73a';
my $s6_v1_md5 = 'c512d2c68977009f85fc9a9334b275f4';
my $s7_v1_md5 = '6a9f1cb110afed33867138289c37521e';
my $s8_v1_md5 = 'fe3fad997a01568060a4577de47fc6a8';


# kaltura:
$t->write_file('a.json', http_content(http_get('/proxy/playlistjson/playlist//kalvod/hls/seg-330574924-a1.ts')));
$t->write_file('v.json', http_content(http_get('/proxy/playlistjson/playlist//kalvod/hls/seg-330574924-v1.ts')));
$t->write_file('av.json', http_content(http_get('/proxy/playlistjson/playlist//kalvod/hls/seg-330574924-v1-a1.ts')));

check_file('/kalvod/hls', '', 'kaltura', 'seg-330574921-v1-a1.ts', '', '86872ec74733c66e222b5b568bfe954d', $s1_v1_md5, $s1_a1_md5);
check_file('/kalvod/hls', '', 'kaltura', 'seg-330574922-v1-a1.ts', '', '7232b0ccbb68307bc694e32529876c92', $s2_v1_md5, $s2_a1_md5);
check_file('/kalvod/hls', '', 'kaltura', 'seg-330574923-v1-a1.ts', '', '97de42e0457e561abb9177c3752e0c4f', $s3_v1_md5, $s3_a1_md5);
check_file('/kalvod/hls', '', 'kaltura', 'seg-330574924-v1-a1.ts', '', 'e7c426cf88382bf2699458bb580a87ae', $s4_v1_md5, $s4_a1_md5);
check_file('/kalvod/hls', '', 'kaltura', 'seg-330574925-v1-a1.ts', '', 'e5e8a2a459d23fcaab2a72fe5d3cfc74', $s5_v1_md5, $s5_a1_md5);
check_file('/kalvod/hls', '', 'kaltura', 'seg-330574926-v1-a1.ts', '', '7f53bd5943bf9d6f345c32a84162427f', $s6_v1_md5, $s6_a1_md5);
check_file('/kalvod/hls', '', 'kaltura', 'seg-330574927-v1-a1.ts', '', '4c6504929fbc015dd7bdb2cb3779479a', $s7_v1_md5, $s7_a1_md5);
check_file('/kalvod/hls', '', 'kaltura', 'seg-330574928-v1-a1.ts', '', 'd43683e9fe9906e4c5f1b5ff88fc99ac', $s8_v1_md5, $s8_a1_md5);

check_file('/kalvod/hls', '', 'kaltura', 'seg-330574924-a1.ts', '', 'c7c7050211e6c05442b5259db0af9f70', $empty_md5, $s4_a1_md5);
check_file('/kalvod/hls', '', 'kaltura', 'seg-330574924-v1.ts', '', '8aea0ca63432ef6d126fa5a0729c9ffc', $s4_v1_md5, $empty_md5);

check_file('/kalvod/dash', '', 'kaltura', 'init-v1.mp4', '', '32818f9fe0aefa4368857d0b7b822852', $empty_md5, $empty_md5);
check_file('/kalvod/dash', '', 'kaltura', 'init-a1.mp4', '', 'd19e1c538a3993f8d100329db8bd1d93', $empty_md5, $empty_md5);

check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574921-v1.m4s', 'init-v1.mp4', '17d65235e1630c286fa704e67fd9976f', $s1_v1_md5, $empty_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574922-v1.m4s', 'init-v1.mp4', '4e4f9a9aa7fcd69a9ca4d68908249a2c', $s2_v1_md5, $empty_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574923-v1.m4s', 'init-v1.mp4', '0351bbe0fb178fac139d95ee8a576c4d', $s3_v1_md5, $empty_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574924-v1.m4s', 'init-v1.mp4', '80ff936e2aeea6a9a4428fc5a14689e9', $s4_v1_md5, $empty_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574925-v1.m4s', 'init-v1.mp4', '6bf6343eaa8e572e2b61d454e9d672a1', $s5_v1_md5, $empty_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574926-v1.m4s', 'init-v1.mp4', '58167f4b0a81209af742900c7a728646', $s6_v1_md5, $empty_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574927-v1.m4s', 'init-v1.mp4', '03a3289c5545595ce6642d581994a135', $s7_v1_md5, $empty_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574928-v1.m4s', 'init-v1.mp4', 'cbb6b74f8dc382b59263644d8f39ad08', $s8_v1_md5, $empty_md5);

check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574921-a1.m4s', 'init-a1.mp4', '593609dff1f04d2aa4dbe8444628dddb', $empty_md5, $s1_a1_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574922-a1.m4s', 'init-a1.mp4', '7bfc0bd8988d3bde8b5315d7b4a67e06', $empty_md5, $s2_a1_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574923-a1.m4s', 'init-a1.mp4', '58168c4be57972a9bc46154c093cfc8d', $empty_md5, $s3_a1_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574924-a1.m4s', 'init-a1.mp4', '8cddb395832deef0aed29bd9e61db8a8', $empty_md5, $s4_a1_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574925-a1.m4s', 'init-a1.mp4', '043e32cdf3577f98efa59e624196d6e6', $empty_md5, $s5_a1_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574926-a1.m4s', 'init-a1.mp4', 'ffe383f8d18ce5d8bd93bd78bdd817c8', $empty_md5, $s6_a1_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574927-a1.m4s', 'init-a1.mp4', '060782be3abd5c4209085b1db6b69bcd', $empty_md5, $s7_a1_md5);
check_file('/kalvod/dash', '', 'kaltura', 'fragment-330574928-a1.m4s', 'init-a1.mp4', 'e3714e396713d162b0aaa9f3b60319b5', $empty_md5, $s8_a1_md5);

check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'FragmentInfo(video=16528746100000000)', '', '07c6269d581669c794c7ece3ec3e6051', $empty_md5, $empty_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'FragmentInfo(audio=16528746100000000)', '', 'b38e0f9e9d7715bdcd36249eac01c5b5', $empty_md5, $empty_md5);

check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(video=16528746000000000)', 'init-v1.mp4', '4d41b2e034bfc02105335431f001daec', $s1_v1_md5, $empty_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(video=16528746050000000)', 'init-v1.mp4', '1b59be2df9083c4cc3388c1acaaa0fc6', $s2_v1_md5, $empty_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(video=16528746100000000)', 'init-v1.mp4', '72632e74fdb333f12ef78db8c9937dee', $s3_v1_md5, $empty_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(video=16528746150000000)', 'init-v1.mp4', '7c55cad87b09917e72cc703b5375f8ec', $s4_v1_md5, $empty_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(video=16528746200000000)', 'init-v1.mp4', '96266d59f963503f3cd95f6d11dc279b', $s5_v1_md5, $empty_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(video=16528746250000000)', 'init-v1.mp4', '757c9ca759c986e0eb4e10e01a237307', $s6_v1_md5, $empty_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(video=16528746300000000)', 'init-v1.mp4', '144a0694831565baf5b040d4a65ce945', $s7_v1_md5, $empty_md5); # need "look-ahead" segment, otherwise we get 400 Bad Request
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(video=16528746350000000)', 'init-v1.mp4', '4199149d912bad3b69456a739f5e8fd9', $s8_v1_md5, $empty_md5); # need "look-ahead" segment, otherwise we get 400 Bad Request

check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(audio=16528746000000000)', 'init-a1.mp4', '13b23ae8b94b2c572242993dca6ec2b4', $empty_md5, $s1_a1_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(audio=16528746050000000)', 'init-a1.mp4', 'c6d2fe68f232aa1d7589e0888a0ea0d1', $empty_md5, $s2_a1_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(audio=16528746100000000)', 'init-a1.mp4', '785e8ccb588eb4fb6b11c7a95a0884f3', $empty_md5, $s3_a1_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(audio=16528746150000000)', 'init-a1.mp4', '9feb96ff768f61bb60e4e8f4fd869823', $empty_md5, $s4_a1_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(audio=16528746200000000)', 'init-a1.mp4', 'b3044233187d36f251b836547b6f867c', $empty_md5, $s5_a1_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(audio=16528746250000000)', 'init-a1.mp4', 'ab75a03d019695e6159a48d2139981d6', $empty_md5, $s6_a1_md5);
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(audio=16528746300000000)', 'init-a1.mp4', '329c0c56aa68c8456acaf48252f8e9f7', $empty_md5, $s7_a1_md5); # need "look-ahead" segment, otherwise we get 400 Bad Request
check_file('/kalvod/mss/QualityLevels(388096)', '', 'kaltura', 'Fragments(audio=16528746350000000)', 'init-a1.mp4', '6b4ed62c1d2ed2eb6519a73d30b816e3', $empty_md5, $s8_a1_md5); # need "look-ahead" segment, otherwise we get 400 Bad Request


# packager kaltura imitation worker:
$t->write_file('s.json', http_content(http_get('/proxy/playlistjson/playlist//kalvod/hls/seg-330574924-s1.ts')));

check_file('/pakkalim', '', 'pakkalim', 'seg-330574921-a1.ts', '', 'c074baa20e438f29ddbd4e657a1985b6', $empty_md5, $s1_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574922-a1.ts', '', '6146ea4ea0ff52bb80181917cc10c334', $empty_md5, $s2_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574923-a1.ts', '', '6594b74fa0107eb4ec600594ad6adefb', $empty_md5, $s3_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574924-a1.ts', '', 'e18d016997b69eee767316d042764521', $empty_md5, $s4_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574925-a1.ts', '', 'e2f9c54f99baba0e0ebcdb1e15c598ae', $empty_md5, $s5_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574926-a1.ts', '', '9beeb0f86b19a7434f7081f0dc6864e7', $empty_md5, $s6_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574927-a1.ts', '', 'd4aea88a7b5524d4e377fcf575408481', $empty_md5, $s7_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574928-a1.ts', '', '932c435a4d1f6404ee962f0cba5213f7', $empty_md5, $s8_a1_md5);

check_file('/pakkalim', '', 'pakkalim', 'seg-330574921-v1.ts', '', '85fe95bbc30eb1f016a80c3705832107', $s1_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574922-v1.ts', '', '4a301c32d9087d7b3ba7e81fc7432db5', $s2_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574923-v1.ts', '', '9dca3be2681bb17e3ab322303f1abee0', $s3_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574924-v1.ts', '', '45489ac6bac8ff7042cd444c5e1da33c', $s4_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574925-v1.ts', '', 'ff866c3e8c1388487d4552a3e7ae0044', $s5_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574926-v1.ts', '', '8546b2610c5b5372053c1b5cfaa4d04c', $s6_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574927-v1.ts', '', '9ae5966b1fba35cf1e4e1e357afa798b', $s7_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574928-v1.ts', '', '2abebe8c723d20aa9989f62f341cb16f', $s8_v1_md5, $empty_md5);

check_file('/pakkalim', '', 'pakkalim', 'init-v1.mp4', '', '16bb12e257d43feb44214d3c4d6d506f', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'init-a1.mp4', '', 'ed0eb82bed1a2e2516a762d8affa4665', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'init-s1.mp4', '', '8bc39c902abe425e0b43d1734a8226b6', $empty_md5, $empty_md5);

check_file('/pakkalim', '', 'pakkalim', 'fragment-330574921-v1.m4s', 'init-v1.mp4', '3cf1a215c108876472df6d314bcc5695', $s1_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574922-v1.m4s', 'init-v1.mp4', '46d6301b3780be641564a5502eda0728', $s2_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574923-v1.m4s', 'init-v1.mp4', '77fc97c28008913b0ab202c62a07875f', $s3_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574924-v1.m4s', 'init-v1.mp4', 'efe21b22ecc47c222d7bb69f6caf3c59', $s4_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574925-v1.m4s', 'init-v1.mp4', '404785b6bc2cfea192979747608450aa', $s5_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574926-v1.m4s', 'init-v1.mp4', '78b69f045a61cf5ef77611cf6c741a79', $s6_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574927-v1.m4s', 'init-v1.mp4', '85552db6811bcff3cd4bef47d7830dfd', $s7_v1_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574928-v1.m4s', 'init-v1.mp4', '4f049989b51a5df3fdd49fcdbb8229ca', $s8_v1_md5, $empty_md5);

check_file('/pakkalim', '', 'pakkalim', 'fragment-330574921-a1.m4s', 'init-a1.mp4', '1a91177c4b4d720ae5bf08c93fbb3194', $empty_md5, $s1_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574922-a1.m4s', 'init-a1.mp4', '5cacdc3a0b03d41587f0a5458d25f3cd', $empty_md5, $s2_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574923-a1.m4s', 'init-a1.mp4', 'bb342ffdf5da03175703e289dada6ba0', $empty_md5, $s3_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574924-a1.m4s', 'init-a1.mp4', '5b030969699b8c4810e297e9b06d3896', $empty_md5, $s4_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574925-a1.m4s', 'init-a1.mp4', 'db97d15663d7784d6f2642357196a9c4', $empty_md5, $s5_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574926-a1.m4s', 'init-a1.mp4', '00d8330819b2e72018d101f845e79a82', $empty_md5, $s6_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574927-a1.m4s', 'init-a1.mp4', 'e98c238c6ce2f829a9ed26407c8ffd1c', $empty_md5, $s7_a1_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574928-a1.m4s', 'init-a1.mp4', '651d50a27d5d26ccc025f758d6a9ac45', $empty_md5, $s8_a1_md5);

check_file('/pakkalim', '', 'pakkalim', 'fragment-330574921-s1.m4s', 'init-s1.mp4', '0d76a1b8d49912418d78d25400ce0f3f', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574922-s1.m4s', 'init-s1.mp4', '54df2f5e4e8bcfe28d4ebaadd92aebe9', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574923-s1.m4s', 'init-s1.mp4', 'bca043b919c13c6ac3b1d30e74312924', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574924-s1.m4s', 'init-s1.mp4', '3c051e9648ed56de752965ae2503bef9', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574925-s1.m4s', 'init-s1.mp4', '42911def1b4f9cbb673807e4a82d9b49', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574926-s1.m4s', 'init-s1.mp4', 'a514effdea72449be75c1c576d04a007', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574927-s1.m4s', 'init-s1.mp4', '526a9c121ce9a88c02a8cc207ed6fba8', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'fragment-330574928-s1.m4s', 'init-s1.mp4', '53abf460a0da1106e17883494ade2590', $empty_md5, $empty_md5);

check_file('/pakkalim', '', 'pakkalim', 'seg-330574921-s1.vtt', '', 'd43db85324d07663ca9616a3d0a2b371', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574922-s1.vtt', '', 'd43db85324d07663ca9616a3d0a2b371', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574923-s1.vtt', '', 'd2f9e6247e764390d5a96b8c6efef64a', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574924-s1.vtt', '', '69ead49a6cd117ad326d13735bb3c831', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574925-s1.vtt', '', '48a17dbe2533524a48810de3ef029c7a', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574926-s1.vtt', '', 'd43db85324d07663ca9616a3d0a2b371', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574927-s1.vtt', '', 'd43db85324d07663ca9616a3d0a2b371', $empty_md5, $empty_md5);
check_file('/pakkalim', '', 'pakkalim', 'seg-330574928-s1.vtt', '', 'd43db85324d07663ca9616a3d0a2b371', $empty_md5, $empty_md5);

# packager vod intervals worker:
$t->write_file('pi.json', http_content(http_get('/proxy/json_meta_intervals')));

check_file('/pakvod/intervals/1/2/3/4/aid0vid0', '', 'packager-intervals', 'seg-1-v1-a1.ts', '', '1ee8268e7a546358274c86c16b2706af', $s1_v1_md5, $s1_a1_md5);
check_file('/pakvod/intervals/1/2/3/4/aid0vid0', '', 'packager-intervals', 'seg-2-v1-a1.ts', '', 'a6e19554970d0e6492bed91aa7810328', $s2_v1_md5, $s2_a1_md5);
check_file('/pakvod/intervals/1/2/3/4/aid0vid0', '', 'packager-intervals', 'seg-3-v1-a1.ts', '', 'ffa1660f40cd1ca385fcc305371683f5', $s3_v1_md5, $s3_a1_md5);
check_file('/pakvod/intervals/1/2/3/4/aid0vid0', '', 'packager-intervals', 'seg-4-v1-a1.ts', '', '2577d264cce4c90f2ed383a814173ae3', $s4_v1_md5, $s4_a1_md5);
check_file('/pakvod/intervals/1/2/3/4/aid0vid0', '', 'packager-intervals', 'seg-5-v1-a1.ts', '', '2475b59c10bead09aa8018780c7d8f0e', $s5_v1_md5, $s5_a1_md5);
check_file('/pakvod/intervals/1/2/3/4/aid0vid0', '', 'packager-intervals', 'seg-6-v1-a1.ts', '', 'd4b7addd9c0dd8d12d03bdeb59d94725', $s6_v1_md5, $s6_a1_md5);
check_file('/pakvod/intervals/1/2/3/4/aid0vid0', '', 'packager-intervals', 'seg-7-v1-a1.ts', '', 'ef08a3ba5f00f7bda0253533c992fb5c', $s7_v1_md5, $s7_a1_md5);
check_file('/pakvod/intervals/1/2/3/4/aid0vid0', '', 'packager-intervals', 'seg-8-v1-a1.ts', '', '288185e83e02aafec4625f56cea67f1c', $s8_v1_md5, $s8_a1_md5);

check_file('/pakvod/intervals/1/2/3/4/vid0', '', 'packager-intervals', 'init-v1.mp4', '', '16bb12e257d43feb44214d3c4d6d506f', $empty_md5, $empty_md5);
check_file('/pakvod/intervals/1/2/3/4/aid0', '', 'packager-intervals', 'init-a1.mp4', '', 'ed0eb82bed1a2e2516a762d8affa4665', $empty_md5, $empty_md5);

# packager vod worker:
$t->write_file('p.json', http_content(http_get('/proxy/json_meta')));

check_file('/pakvod/nodrm/1/2/3/4/aid0vid0', '', 'packager', 'seg-1-v1-a1.ts', '', '1ee8268e7a546358274c86c16b2706af', $s1_v1_md5, $s1_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0vid0', '', 'packager', 'seg-2-v1-a1.ts', '', 'a6e19554970d0e6492bed91aa7810328', $s2_v1_md5, $s2_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0vid0', '', 'packager', 'seg-3-v1-a1.ts', '', 'ffa1660f40cd1ca385fcc305371683f5', $s3_v1_md5, $s3_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0vid0', '', 'packager', 'seg-4-v1-a1.ts', '', '2577d264cce4c90f2ed383a814173ae3', $s4_v1_md5, $s4_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0vid0', '', 'packager', 'seg-5-v1-a1.ts', '', '2475b59c10bead09aa8018780c7d8f0e', $s5_v1_md5, $s5_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0vid0', '', 'packager', 'seg-6-v1-a1.ts', '', 'd4b7addd9c0dd8d12d03bdeb59d94725', $s6_v1_md5, $s6_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0vid0', '', 'packager', 'seg-7-v1-a1.ts', '', 'ef08a3ba5f00f7bda0253533c992fb5c', $s7_v1_md5, $s7_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0vid0', '', 'packager', 'seg-8-v1-a1.ts', '', '288185e83e02aafec4625f56cea67f1c', $s8_v1_md5, $s8_a1_md5);

check_file('/pakvod/nodrm/1/2/3/4/vid0', '', 'packager', 'init-v1.mp4', '', '16bb12e257d43feb44214d3c4d6d506f', $empty_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', '', 'packager', 'init-a1.mp4', '', 'ed0eb82bed1a2e2516a762d8affa4665', $empty_md5, $empty_md5);

check_file('/pakvod/nodrm/1/2/3/4/vid0', '', 'packager', 'fragment-1-v1.m4s', 'init-v1.mp4', '6fd67901b181b872930bbf5e621931ad', $s1_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', '', 'packager', 'fragment-2-v1.m4s', 'init-v1.mp4', 'ba6c8c69421fde191a1f2aa49fc5c519', $s2_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', '', 'packager', 'fragment-3-v1.m4s', 'init-v1.mp4', '39a5338fd38aca11242e532fe61441d8', $s3_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', '', 'packager', 'fragment-4-v1.m4s', 'init-v1.mp4', '7345cbad686c450dbb17d2262c061bfb', $s4_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', '', 'packager', 'fragment-5-v1.m4s', 'init-v1.mp4', '81643cb117737e01b933c83e746ca5d3', $s5_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', '', 'packager', 'fragment-6-v1.m4s', 'init-v1.mp4', 'f8a86dd2d60f59b2750776c7afb66b4c', $s6_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', '', 'packager', 'fragment-7-v1.m4s', 'init-v1.mp4', '7c3ff28de26456f59f42f57903de0878', $s7_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', '', 'packager', 'fragment-8-v1.m4s', 'init-v1.mp4', '59b42848ec221b7aded6ff8df41970df', $s8_v1_md5, $empty_md5);

check_file('/pakvod/nodrm/1/2/3/4/aid0', '', 'packager', 'fragment-1-a1.m4s', 'init-a1.mp4', '7a8d0f711c97df365d6122ca02f1fabb', $empty_md5, $s1_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', '', 'packager', 'fragment-2-a1.m4s', 'init-a1.mp4', 'e8808da4dfc495751ac234b20970d408', $empty_md5, $s2_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', '', 'packager', 'fragment-3-a1.m4s', 'init-a1.mp4', 'e14577bfd2c989d8e520a47b910f311a', $empty_md5, $s3_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', '', 'packager', 'fragment-4-a1.m4s', 'init-a1.mp4', '3cf27382d05f321e9718fde756172705', $empty_md5, $s4_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', '', 'packager', 'fragment-5-a1.m4s', 'init-a1.mp4', '97b00940d3efc03ca30886fd23fe4201', $empty_md5, $s5_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', '', 'packager', 'fragment-6-a1.m4s', 'init-a1.mp4', 'e68c696faecdb44e81f56cc8d63397e3', $empty_md5, $s6_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', '', 'packager', 'fragment-7-a1.m4s', 'init-a1.mp4', '6d5d0a09f318b8f1dd20f508b084eda7', $empty_md5, $s7_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', '', 'packager', 'fragment-8-a1.m4s', 'init-a1.mp4', 'adfeb7660900aaeb4e102410ea586980', $empty_md5, $s8_a1_md5);

check_file('/pakvod/nodrm/1/2/3/4/vid0', 'bufsize=0', 'packager-cmaf', 'init-v1.mp4', '', '16bb12e257d43feb44214d3c4d6d506f', $empty_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', 'bufsize=0', 'packager-cmaf', 'init-a1.mp4', '', 'ed0eb82bed1a2e2516a762d8affa4665', $empty_md5, $empty_md5);

check_file('/pakvod/nodrm/1/2/3/4/vid0', 'bufsize=0', 'packager-cmaf', 'fragment-1-v1.m4s', 'init-v1.mp4', '6d9b672053407508195b6be7f8d2412b', $s1_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', 'bufsize=0', 'packager-cmaf', 'fragment-2-v1.m4s', 'init-v1.mp4', '8392f5b9cf309b223ee060402532b54c', $s2_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', 'bufsize=0', 'packager-cmaf', 'fragment-3-v1.m4s', 'init-v1.mp4', '3387b7b6df821c8756d597d9b9954644', $s3_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', 'bufsize=0', 'packager-cmaf', 'fragment-4-v1.m4s', 'init-v1.mp4', '30c54fd0596b8fff4867577a93cb9425', $s4_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', 'bufsize=0', 'packager-cmaf', 'fragment-5-v1.m4s', 'init-v1.mp4', 'c034c282636da157196f22ca45810220', $s5_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', 'bufsize=0', 'packager-cmaf', 'fragment-6-v1.m4s', 'init-v1.mp4', '72b12bd480448e057a006ce6790cd5bb', $s6_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', 'bufsize=0', 'packager-cmaf', 'fragment-7-v1.m4s', 'init-v1.mp4', 'fbcca2bd678df3e512cf744ec15b95ff', $s7_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/2/3/4/vid0', 'bufsize=0', 'packager-cmaf', 'fragment-8-v1.m4s', 'init-v1.mp4', '7f5008b1b8cbe73a530c3c6856f39abb', $s8_v1_md5, $empty_md5);

check_file('/pakvod/nodrm/1/2/3/4/aid0', 'bufsize=0', 'packager-cmaf', 'fragment-1-a1.m4s', 'init-a1.mp4', 'fad5e74092122933928cc31f18ccc468', $empty_md5, $s1_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', 'bufsize=0', 'packager-cmaf', 'fragment-2-a1.m4s', 'init-a1.mp4', '410caf9a785568a56e87290a88656b53', $empty_md5, $s2_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', 'bufsize=0', 'packager-cmaf', 'fragment-3-a1.m4s', 'init-a1.mp4', 'f6439e95e453974a8c7c17da80e87caa', $empty_md5, $s3_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', 'bufsize=0', 'packager-cmaf', 'fragment-4-a1.m4s', 'init-a1.mp4', '5c94187991b609b9dc381a978852bb74', $empty_md5, $s4_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', 'bufsize=0', 'packager-cmaf', 'fragment-5-a1.m4s', 'init-a1.mp4', 'fd35762edeb458800ac7c98cea3e1691', $empty_md5, $s5_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', 'bufsize=0', 'packager-cmaf', 'fragment-6-a1.m4s', 'init-a1.mp4', 'd094b984e6454d18eeb0e2f8b0f1a5a7', $empty_md5, $s6_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', 'bufsize=0', 'packager-cmaf', 'fragment-7-a1.m4s', 'init-a1.mp4', 'd8d4dca828f2946bcfbcbd40f4c383cc', $empty_md5, $s7_a1_md5);
check_file('/pakvod/nodrm/1/2/3/4/aid0', 'bufsize=0', 'packager-cmaf', 'fragment-8-a1.m4s', 'init-a1.mp4', 'db5a69ac4aeea505ba8b1d8e89fbea6b', $empty_md5, $s8_a1_md5);

#drm:

my $s1_v1_md5_drm_hls = 'b0e54eefb75c545b915f811eb13eebab';
my $s1_a1_md5_drm_hls_kaltura = '9a1253e7155eb5ecebf0f5b8a4399713';
my $s1_a1_md5_drm_hls_packager = '5ed9cf6e9dbdcafa7894416cfc938483';

my $s1_v1_md5_drm_dash_kaltura = '8068bc331774ec23077b609bd3beb3f3';
my $s1_a1_md5_drm_dash_kaltura = 'f33cd14bf03cec161eb46cd5be8a4ebd';

my $s1_v1_md5_drm_cbcs_packager = '07c50c081eb6e36a6ecef34712f12550';
my $s1_a1_md5_drm_cbcs_packager = '3542f5695922862093180b7ac858e416';

my $s1_v1_md5_drm_cenc_packager = 'f605fa5ed0589863d0f61e5c7d591bbe';
my $s1_a1_md5_drm_cenc_packager = '002d363de684a2f94af342e9612758ad';

check_file('/kalvod/hlsdrm', '', 'kaltura-drm', 'seg-330574921-v1-a1.ts', '', '5b050e59c749de921ca1b75cdac603fc', $s1_v1_md5_drm_hls, $s1_a1_md5_drm_hls_kaltura);
check_file('/kalvod/hlsdrm', '', 'kaltura-drm', 'seg-330574921-v1.ts', '', 'd7ac8367e12465dca5fcbaed6e050bb6', $s1_v1_md5_drm_hls, $empty_md5);
check_file('/kalvod/hlsdrm', '', 'kaltura-drm', 'seg-330574921-a1.ts', '', 'e83ae79f908c7a5a1723a2d49e993f81', $empty_md5, $s1_a1_md5_drm_hls_kaltura);

check_file('/kalvod/dashdrm', '', 'kaltura-drm', 'init-v1.mp4', '', 'b56ec98c1ee9cb3ceedb8e7b9c395a85', $empty_md5, $empty_md5);
check_file('/kalvod/dashdrm', '', 'kaltura-drm', 'init-a1.mp4', '', 'a082df4b321460a32de3f245a85e39ad', $empty_md5, $empty_md5);

check_file('/kalvod/dashdrm', '', 'kaltura-drm', 'fragment-330574921-v1.m4s', 'init-v1.mp4', '51e74462fa66a23e252012f1b7532aa4', $s1_v1_md5_drm_dash_kaltura, $empty_md5);
check_file('/kalvod/dashdrm', '', 'kaltura-drm', 'fragment-330574921-a1.m4s', 'init-a1.mp4', 'db24a720f83cef1051f173cb788440cb', $empty_md5, $s1_a1_md5_drm_dash_kaltura);

check_file('/kalvod/mssdrm/QualityLevels(388096)', '', 'kaltura-drm', 'FragmentInfo(video=16528746100000000)', '', '08520a2f9aafbea144a175b424e4b36d', $empty_md5, $empty_md5);
check_file('/kalvod/mssdrm/QualityLevels(388096)', '', 'kaltura-drm', 'FragmentInfo(audio=16528746100000000)', '', '6978d4bed31d90ab4b838c7c204066c3', $empty_md5, $empty_md5);

check_file('/kalvod/mssdrm/QualityLevels(388096)', '', 'kaltura-drm', 'Fragments(video=16528746000000000)', 'init-v1.mp4', '33f116873ab3b58f2d6db2153287021d', $s1_v1_md5_drm_dash_kaltura, $empty_md5);

check_file('/kalvod/mssdrm/QualityLevels(388096)', '', 'kaltura-drm', 'Fragments(audio=16528746000000000)', 'init-a1.mp4', '2776670057ecf8b0ab6853ccce31f949', $empty_md5, $s1_a1_md5_drm_dash_kaltura);


check_file('/pakvod/drmcbcs/1/2/3/4/aid0vid0', '', 'packager-drm', 'seg-1-v1-a1.ts', '', 'b00b6b18e6baa42fd4c0b695ecfcaf5b', $s1_v1_md5_drm_hls, $s1_a1_md5_drm_hls_packager);
check_file('/pakvod/drmcbcs/1/2/3/4/vid0', '', 'packager-drm', 'seg-1-v1.ts', '', 'd857e4311ab2674d601d84628521e801', $s1_v1_md5_drm_hls, $empty_md5);
check_file('/pakvod/drmcbcs/1/2/3/4/aid0', '', 'packager-drm', 'seg-1-a1.ts', '', '28a173ccbd85a4c6ed52a4d36a2f33a5', $empty_md5, $s1_a1_md5_drm_hls_packager);

check_file('/pakvod/drmcenc/1/2/3/4/vid0', '', 'packager-drm-cenc', 'init-v1.mp4', '', 'de7ed1225757426a6ceb2fb4c2329167', $empty_md5, $empty_md5);
check_file('/pakvod/drmcenc/1/2/3/4/aid0', '', 'packager-drm-cenc', 'init-a1.mp4', '', '8fb8ee225de45be40f3651371d0552a7', $empty_md5, $empty_md5);

check_file('/pakvod/drmcenc/1/2/3/4/vid0', '', 'packager-drm-cenc', 'fragment-1-v1.m4s', 'init-v1.mp4', '98ce9c3018379fb492823e3b8689b69d', $s1_v1_md5_drm_cenc_packager, $empty_md5);
check_file('/pakvod/drmcenc/1/2/3/4/aid0', '', 'packager-drm-cenc', 'fragment-1-a1.m4s', 'init-a1.mp4', 'd605da4b3d13f82c32e64cc4667d6544', $empty_md5, $s1_a1_md5_drm_cenc_packager);

check_file('/pakvod/drmcbcs/1/2/3/4/vid0', '', 'packager-drm-cbcs', 'init-v1.mp4', '', '43b2818c7ab65bae4f7f532c07c45414', $empty_md5, $empty_md5);
check_file('/pakvod/drmcbcs/1/2/3/4/aid0', '', 'packager-drm-cbcs', 'init-a1.mp4', '', '667570bea9c054b7b6310c4669f6fb77', $empty_md5, $empty_md5);

check_file('/pakvod/drmcbcs/1/2/3/4/vid0', '', 'packager-drm-cbcs', 'fragment-1-v1.m4s', 'init-v1.mp4', '7d55afda1b3f9a3d586363a403a69020', $s1_v1_md5_drm_cbcs_packager, $empty_md5);
check_file('/pakvod/drmcbcs/1/2/3/4/aid0', '', 'packager-drm-cbcs', 'fragment-1-a1.m4s', 'init-a1.mp4', '231bc2fc9bd895a9560e3c5fcf829661', $empty_md5, $s1_a1_md5_drm_cbcs_packager);



check_file('/pakvod/drmcenc/1/2/3/4/vid0', 'bufsize=0', 'packager-drm-cenc-cmaf', 'init-v1.mp4', '', 'de7ed1225757426a6ceb2fb4c2329167', $empty_md5, $empty_md5);
check_file('/pakvod/drmcenc/1/2/3/4/aid0', 'bufsize=0', 'packager-drm-cenc-cmaf', 'init-a1.mp4', '', '8fb8ee225de45be40f3651371d0552a7', $empty_md5, $empty_md5);

check_file('/pakvod/drmcenc/1/2/3/4/vid0', 'bufsize=0', 'packager-drm-cenc-cmaf', 'fragment-1-v1.m4s', 'init-v1.mp4', 'b9e0beb319df2f2077c6a78d8b68fe47', $s1_v1_md5_drm_cenc_packager, $empty_md5);
check_file('/pakvod/drmcenc/1/2/3/4/aid0', 'bufsize=0', 'packager-drm-cenc-cmaf', 'fragment-1-a1.m4s', 'init-a1.mp4', 'cebcc0147fda46b5c9d10df88515165a', $empty_md5, $s1_a1_md5_drm_cenc_packager);

check_file('/pakvod/drmcbcs/1/2/3/4/vid0', 'bufsize=0', 'packager-drm-cbcs-cmaf', 'init-v1.mp4', '', '43b2818c7ab65bae4f7f532c07c45414', $empty_md5, $empty_md5);
check_file('/pakvod/drmcbcs/1/2/3/4/aid0', 'bufsize=0', 'packager-drm-cbcs-cmaf', 'init-a1.mp4', '', '667570bea9c054b7b6310c4669f6fb77', $empty_md5, $empty_md5);

check_file('/pakvod/drmcbcs/1/2/3/4/vid0', 'bufsize=0', 'packager-drm-cbcs-cmaf', 'fragment-1-v1.m4s', 'init-v1.mp4', '6ce54ff8c72774a60791ee10e9957ccf', $s1_v1_md5_drm_cbcs_packager, $empty_md5);
check_file('/pakvod/drmcbcs/1/2/3/4/aid0', 'bufsize=0', 'packager-drm-cbcs-cmaf', 'fragment-1-a1.m4s', 'init-a1.mp4', 'fd3529f848303133557450510e3f57b6', $empty_md5, $s1_a1_md5_drm_cbcs_packager);




#hevc_ec3:

my $he_s1_a1_md5 = '398be89a94259181a4277ed995197169';
my $he_s2_a1_md5 = 'b9e1579cf4c240dc413dec8c9901d172';

my $he_s1_v1_md5 = '6ac5b015a0e174bc42131d550221d4b9';
my $he_s2_v1_md5 = 'e435ad2fdda2c1352f59f2f8148a735c';

my $he_s1_v1_md5_kaltura_ts = 'ebdc10c08cf7e8cc6afa7e379df5f773';
my $he_s2_v1_md5_kaltura_ts = '5ae2c891cb8fcb6f01fe1752b03f75b6';

check_file('/pakvod/nodrm/1/hevc_ec3/3/4/aid0', '', 'packager-he', 'seg-1-a1.ts', '', '41fd15336add982abb802cb8487f1ae5', $empty_md5, $he_s1_a1_md5);
check_file('/pakvod/nodrm/1/hevc_ec3/3/4/aid0', '', 'packager-he', 'seg-2-a1.ts', '', '634d66527822582dd5a4238fc45be74d', $empty_md5, $he_s2_a1_md5);

check_file('/pakvod/nodrm/1/hevc_ec3/3/4/vid0', '', 'packager-he', 'init-v1.mp4', '', '6dacd9e97d7e6b710ed4867f80f398ff', $empty_md5, $empty_md5);
check_file('/pakvod/nodrm/1/hevc_ec3/3/4/aid0', '', 'packager-he', 'init-a1.mp4', '', 'c42032028bddb514b1215a3211be1bc9', $empty_md5, $empty_md5);

check_file('/pakvod/nodrm/1/hevc_ec3/3/4/vid0', '', 'packager-he', 'fragment-1-v1.m4s', 'init-v1.mp4', '52c73fc8bb026fd67ee663b4a3482634', $he_s1_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/hevc_ec3/3/4/vid0', '', 'packager-he', 'fragment-2-v1.m4s', 'init-v1.mp4', '52f19cd2a84f31354bea2b4e0ef57101', $he_s2_v1_md5, $empty_md5);

check_file('/pakvod/nodrm/1/hevc_ec3/3/4/aid0', '', 'packager-he', 'fragment-1-a1.m4s', 'init-a1.mp4', '2d8891f6954975f0a7f58dc8f0db3170', $empty_md5, $he_s1_a1_md5);
check_file('/pakvod/nodrm/1/hevc_ec3/3/4/aid0', '', 'packager-he', 'fragment-2-a1.m4s', 'init-a1.mp4', '6041ac92b395743c1d99b6c87a035bdf', $empty_md5, $he_s2_a1_md5);


check_file('/pakvod/nodrm/1/hevc_ec3/3/4/vid0', 'bufsize=0', 'packager-he-cmaf', 'init-v1.mp4', '', '6dacd9e97d7e6b710ed4867f80f398ff', $empty_md5, $empty_md5);
check_file('/pakvod/nodrm/1/hevc_ec3/3/4/aid0', 'bufsize=0', 'packager-he-cmaf', 'init-a1.mp4', '', 'c42032028bddb514b1215a3211be1bc9', $empty_md5, $empty_md5);

check_file('/pakvod/nodrm/1/hevc_ec3/3/4/vid0', 'bufsize=0', 'packager-he-cmaf', 'fragment-1-v1.m4s', 'init-v1.mp4', 'bd96d769c617eec2e4c88315405f81c4', $he_s1_v1_md5, $empty_md5);
check_file('/pakvod/nodrm/1/hevc_ec3/3/4/vid0', 'bufsize=0', 'packager-he-cmaf', 'fragment-2-v1.m4s', 'init-v1.mp4', '663af8bacd0becae0accf1967e86e55d', $he_s2_v1_md5, $empty_md5);

check_file('/pakvod/nodrm/1/hevc_ec3/3/4/aid0', 'bufsize=0', 'packager-he-cmaf', 'fragment-1-a1.m4s', 'init-a1.mp4', '14178335aa011121c4d17563c6196ad2', $empty_md5, $he_s1_a1_md5);
check_file('/pakvod/nodrm/1/hevc_ec3/3/4/aid0', 'bufsize=0', 'packager-he-cmaf', 'fragment-2-a1.m4s', 'init-a1.mp4', 'fc4eba60566ffe1d7bd3e3a07154e1b2', $empty_md5, $he_s2_a1_md5);


check_file('/kalvod/hls/hevc_ec3', '', 'kaltura-he', 'seg-1-v1.ts', '', '4db95633658a9d534a437e7d4b1fb3fb', $he_s1_v1_md5_kaltura_ts, $empty_md5);
check_file('/kalvod/hls/hevc_ec3', '', 'kaltura-he', 'seg-2-v1.ts', '', '7fb594f867e6e67f14ceedfb2458db09', $he_s2_v1_md5_kaltura_ts, $empty_md5);
check_file('/kalvod/hls/hevc_ec3', '', 'kaltura-he', 'seg-1-a1.ts', '', '01ca5531966e012a29a8f4a1c5d9f185', $empty_md5, $he_s1_a1_md5);
check_file('/kalvod/hls/hevc_ec3', '', 'kaltura-he', 'seg-2-a1.ts', '', '49efb129c8ea12974c4c5127b81900c1', $empty_md5, $he_s2_a1_md5);

check_file('/kalvod/dash/hevc_ec3', '', 'kaltura-he', 'init-v1.mp4', '', 'b38b99c9c6f72058bf5acbc4f9a9ba3e', $empty_md5, $empty_md5);
check_file('/kalvod/dash/hevc_ec3', '', 'kaltura-he', 'init-a1.mp4', '', '6ecab9230aaee5b6b2081cc3a26a4d75', $empty_md5, $empty_md5);

check_file('/kalvod/dash/hevc_ec3', '', 'kaltura-he', 'fragment-1-v1.m4s', 'init-v1.mp4', '92a30fdf1487436a5f465cf9c509238e', $he_s1_v1_md5, $empty_md5);
check_file('/kalvod/dash/hevc_ec3', '', 'kaltura-he', 'fragment-2-v1.m4s', 'init-v1.mp4', 'b78bde9426675c6047bfb2f261bfce02', $he_s2_v1_md5, $empty_md5);

check_file('/kalvod/dash/hevc_ec3', '', 'kaltura-he', 'fragment-1-a1.m4s', 'init-a1.mp4', 'cdc8d9506a61675e3325276ccd01e998', $empty_md5, $he_s1_a1_md5);
check_file('/kalvod/dash/hevc_ec3', '', 'kaltura-he', 'fragment-2-a1.m4s', 'init-a1.mp4', 'f60fa89cce3d8c80f1e2ca8b84cb8092', $empty_md5, $he_s2_a1_md5);

# TODO: fix mss with hevc & ec3 (right now kaltura response with 400)
# check_file('/kalvod/mss/hevc_ec3/QualityLevels(388096)', '', 'kaltura-he', 'FragmentInfo(video=150000000)', '', 'tbd', $empty_md5, $empty_md5);
# check_file('/kalvod/mss/hevc_ec3/QualityLevels(388096)', '', 'kaltura-he', 'FragmentInfo(audio=150000000)', '', 'tbd', $empty_md5, $empty_md5);

# check_file('/kalvod/mss/hevc_ec3/QualityLevels(388096)', '', 'kaltura-he', 'Fragments(video=100000000)', 'init-v1.mp4', 'tbd', $he_s1_v1_md5, $empty_md5);
# check_file('/kalvod/mss/hevc_ec3/QualityLevels(388096)', '', 'kaltura-he', 'Fragments(video=150000000)', 'init-v1.mp4', 'tbd', $he_s2_v1_md5, $empty_md5);

# check_file('/kalvod/mss/hevc_ec3/QualityLevels(388096)', '', 'kaltura-he', 'Fragments(audio=100000000)', 'init-a1.mp4', 'tbd', $empty_md5, $he_s1_a1_md5);
# check_file('/kalvod/mss/hevc_ec3/QualityLevels(388096)', '', 'kaltura-he', 'Fragments(audio=150000000)', 'init-a1.mp4', 'tbd', $empty_md5, $he_s2_a1_md5);



#hevc_ec3+drm:

my $he_s1_a1_md5_drm_ts = '26bc75450fa14fd624a1c007931c8469';

my $he_s1_v1_md5_dmr_cbcs = '58495150a3c778e5ab674327c340def2';
my $he_s1_a1_md5_dmr_cbcs = 'fd9b08a7060469c5350981f161f25b1b';

my $he_s1_v1_md5_dmr_cenc_packager = 'e12b6ad100b84c20950112a70ab5beda';
my $he_s1_a1_md5_dmr_cenc_packager = '640330e097345a830b96358d59b36260';
my $he_s1_v1_md5_dmr_cenc_kaltura = 'e5d1128df6778993593e35981a9489b0';
my $he_s1_a1_md5_dmr_cenc_kaltura = '6ef02c1815f32fb183cfb5d9e91239e4';

check_file('/pakvod/drmcbcs/1/hevc_ec3/3/4/aid0', '', 'packager-he-drm', 'seg-1-a1.ts', '', '38914222bea509bccbfc410e05a21ee9', $empty_md5, $he_s1_a1_md5_drm_ts);

check_file('/pakvod/drmcbcs/1/hevc_ec3/3/4/vid0', '', 'packager-he-drm-cbcs', 'init-v1.mp4', '', 'cae6a1f4eb5bb58be065c3caa368dca5', $empty_md5, $empty_md5);
check_file('/pakvod/drmcbcs/1/hevc_ec3/3/4/aid0', '', 'packager-he-drm-cbcs', 'init-a1.mp4', '', '7f34ea6a014d7ae628b50b91f26a969d', $empty_md5, $empty_md5);

check_file('/pakvod/drmcbcs/1/hevc_ec3/3/4/vid0', '', 'packager-he-drm-cbcs', 'fragment-1-v1.m4s', 'init-v1.mp4', 'd04e7aea6f20457bc061434735c7c05a', $he_s1_v1_md5_dmr_cbcs, $empty_md5);
check_file('/pakvod/drmcbcs/1/hevc_ec3/3/4/aid0', '', 'packager-he-drm-cbcs', 'fragment-1-a1.m4s', 'init-a1.mp4', '4f1a32aa12a1cde469aea35b3b5bdb2b', $empty_md5, $he_s1_a1_md5_dmr_cbcs);

check_file('/pakvod/drmcenc/1/hevc_ec3/3/4/vid0', '', 'packager-he-drm-cenc', 'init-v1.mp4', '', '39a92c70ead6ab32f7f6deab8fdf0f37', $empty_md5, $empty_md5);
check_file('/pakvod/drmcenc/1/hevc_ec3/3/4/aid0', '', 'packager-he-drm-cenc', 'init-a1.mp4', '', 'dfd9512afaf92b71c58485ea002df24d', $empty_md5, $empty_md5);

check_file('/pakvod/drmcenc/1/hevc_ec3/3/4/vid0', '', 'packager-he-drm-cenc', 'fragment-1-v1.m4s', 'init-v1.mp4', '8dd52d0ec1d38c259af3b4b2be000f7f', $he_s1_v1_md5_dmr_cenc_packager, $empty_md5);
check_file('/pakvod/drmcenc/1/hevc_ec3/3/4/aid0', '', 'packager-he-drm-cenc', 'fragment-1-a1.m4s', 'init-a1.mp4', 'a58e49ad2883fa3f0cbc1022ec4c34fb', $empty_md5, $he_s1_a1_md5_dmr_cenc_packager);


check_file('/pakvod/drmcbcs/1/hevc_ec3/3/4/vid0', 'bufsize=0', 'packager-he-drm-cbcs-cmaf', 'init-v1.mp4', '', 'cae6a1f4eb5bb58be065c3caa368dca5', $empty_md5, $empty_md5);
check_file('/pakvod/drmcbcs/1/hevc_ec3/3/4/aid0', 'bufsize=0', 'packager-he-drm-cbcs-cmaf', 'init-a1.mp4', '', '7f34ea6a014d7ae628b50b91f26a969d', $empty_md5, $empty_md5);

check_file('/pakvod/drmcbcs/1/hevc_ec3/3/4/vid0', 'bufsize=0', 'packager-he-drm-cbcs-cmaf', 'fragment-1-v1.m4s', 'init-v1.mp4', '0ad57d02a282484a3b39fa912bb86bc2', $he_s1_v1_md5_dmr_cbcs, $empty_md5);
check_file('/pakvod/drmcbcs/1/hevc_ec3/3/4/aid0', 'bufsize=0', 'packager-he-drm-cbcs-cmaf', 'fragment-1-a1.m4s', 'init-a1.mp4', '94200e991984942b10a44510a10a6816', $empty_md5, $he_s1_a1_md5_dmr_cbcs);

check_file('/pakvod/drmcenc/1/hevc_ec3/3/4/vid0', 'bufsize=0', 'packager-he-drm-cenc-cmaf', 'init-v1.mp4', '', '39a92c70ead6ab32f7f6deab8fdf0f37', $empty_md5, $empty_md5);
check_file('/pakvod/drmcenc/1/hevc_ec3/3/4/aid0', 'bufsize=0', 'packager-he-drm-cenc-cmaf', 'init-a1.mp4', '', 'dfd9512afaf92b71c58485ea002df24d', $empty_md5, $empty_md5);

check_file('/pakvod/drmcenc/1/hevc_ec3/3/4/vid0', 'bufsize=0', 'packager-he-drm-cenc-cmaf', 'fragment-1-v1.m4s', 'init-v1.mp4', '6f58faf82ec556555070574d48b1cb6b', $he_s1_v1_md5_dmr_cenc_packager, $empty_md5);
check_file('/pakvod/drmcenc/1/hevc_ec3/3/4/aid0', 'bufsize=0', 'packager-he-drm-cenc-cmaf', 'fragment-1-a1.m4s', 'init-a1.mp4', 'b52316c5852da82bcf0b642c19cf9ec9', $empty_md5, $he_s1_a1_md5_dmr_cenc_packager);


check_file('/kalvod/hlsdrm/hevc_ec3', '', 'kaltura-he-drm', 'seg-1-a1.ts', '', 'bd05040ca55c3a8dcbf3e5ef73edf092', $empty_md5, $he_s1_a1_md5_drm_ts);

check_file('/kalvod/dashdrm/hevc_ec3', '', 'kaltura-he-drm-dash', 'init-v1.mp4', '', '6947c6e82d88106d6a98b3884c8b760a', $empty_md5, $empty_md5);
check_file('/kalvod/dashdrm/hevc_ec3', '', 'kaltura-he-drm-dash', 'init-a1.mp4', '', '1d7442393a75c56bcde66625e17d27e1', $empty_md5, $empty_md5);

check_file('/kalvod/dashdrm/hevc_ec3', '', 'kaltura-he-drm-dash', 'fragment-1-v1.m4s', 'init-v1.mp4', '934377595092745306848d388feac597', $he_s1_v1_md5_dmr_cenc_kaltura, $empty_md5);
check_file('/kalvod/dashdrm/hevc_ec3', '', 'kaltura-he-drm-dash', 'fragment-1-a1.m4s', 'init-a1.mp4', '230ae525354dc01dc117cc1ed56bf851', $empty_md5, $he_s1_a1_md5_dmr_cenc_kaltura);

check_file('/kalvod/hlsdrm/hevc_ec3', '', 'kaltura-he-drm-hls', 'init-v1.mp4', '', '341e35384154eea1e9301192c2903cd7', $empty_md5, $empty_md5);
check_file('/kalvod/hlsdrm/hevc_ec3', '', 'kaltura-he-drm-hls', 'init-a1.mp4', '', '2470a1e4b433a785022ed459a9151871', $empty_md5, $empty_md5);

check_file('/kalvod/hlsdrm/hevc_ec3', '', 'kaltura-he-drm-hls', 'seg-1-v1.m4s', 'init-v1.mp4', 'd8389e6933c228a7c11d06819f76769b', $he_s1_v1_md5_dmr_cbcs, $empty_md5);
check_file('/kalvod/hlsdrm/hevc_ec3', '', 'kaltura-he-drm-hls', 'seg-1-a1.m4s', 'init-a1.mp4', '116af3374182a4d0bccebd884af9337a', $empty_md5, $he_s1_a1_md5_dmr_cbcs);



# TODO:
# kaltura mss hevc_ec3 (+drm)


# $t1 = http_get('/proxy/freeze'); # uncomment to freeze test, and be able to investigate requests results saved in test directory, error.log, etc.


# use --inline-diff to see all broken md5:
#   cd nginx/strm-bin; yarmake -tt --inline-diff
