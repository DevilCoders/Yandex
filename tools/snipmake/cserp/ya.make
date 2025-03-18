PROGRAM()

OWNER(
    divankov
    g:snippets
)

SRCS(
    main.cpp
    parseitem.cpp
    parsejson.cpp
    img.cpp
)

PEERDIR(
    library/cpp/string_utils/base64
    tools/snipmake/cserp/idl
    tools/snipmake/argv
    tools/snipmake/snipdat
    library/cpp/json
    library/cpp/svnversion
    contrib/libs/protobuf
    contrib/libs/ImageMagick
    library/cpp/cgiparam
)

END()
