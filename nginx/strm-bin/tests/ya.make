PY3TEST()

OWNER(
    dronimal
    g:strm-admin
)

PEERDIR(
    contrib/nginx/tests
)

DATA(
    arcadia/contrib/nginx/tests/lib
    sbr://3127019487
)

#cahnge number to trigger validate_resource restart in PR
VALIDATE_DATA_RESTART(123456)

DEPENDS(
    nginx/strm-bin
    contrib/libs/ffmpeg-3/bin/ffprobe
    contrib/libs/ffmpeg-3/bin
)

TEST_SRCS(
    test_tap.py
)

TIMEOUT(600)
SIZE(MEDIUM)

# FORK_SUBTESTS()

END()
