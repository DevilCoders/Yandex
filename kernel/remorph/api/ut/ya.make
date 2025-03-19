OWNER(g:remorph)

IF (NOT OS_WINDOWS)

    UNITTEST()

    INCLUDE(${ARCADIA_ROOT}/kernel/remorph/version.cmake)

    CFLAGS(
        -DREMORPH_CODEBASE_VERSION_MAJOR=${REMORPH_EXTVER_MAJOR}
        -DREMORPH_CODEBASE_VERSION_MINOR=${REMORPH_EXTVER_MINOR}
        -DREMORPH_CODEBASE_VERSION_PATCH=${REMORPH_EXTVER_PATCH}
    )

    RUN_PROGRAM(
        dict/gazetteer/compiler -f -q -I ${ARCADIA_ROOT} remorph_api_ut_gzt.gzt remorph_api_ut_gzt.gzt.bin
        IN
            remorph_api_ut_gzt.gzt
            remorph_api_ut_gzt.gztproto
            ${ARCADIA_ROOT}/kernel/gazetteer/proto/base.proto
        OUT_NOAUTO remorph_api_ut_gzt.gzt.bin
    )

    ARCHIVE(
        NAME resources.inc
        ${BINDIR}/remorph_api_ut_gzt.gzt.bin
    )

    SRCS(
        remorph_api_ut.cpp
        remorph_api_ut_gzt.gztproto
        ${BINDIR}/resources.inc
    )

    PEERDIR(
        contrib/libs/protobuf
        kernel/remorph/api
        library/cpp/archive
        library/cpp/json
    )

    END()

ELSE()
    LIBRARY()
    END()

ENDIF()
