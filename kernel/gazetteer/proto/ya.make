PROTO_LIBRARY()

OWNER(
    dmitryno
    g:wizard
    gotmanov
)

SRCS(
    syntax.proto # description of *.gzt format (textual)
    binary.proto # description of *.gzt.bin format (binary)
    base.proto # base gazetteer definitions
)

EXCLUDE_TAGS(GO_PROTO)

END()
