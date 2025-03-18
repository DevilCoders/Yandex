GO_LIBRARY()

OWNER(
    g:strm-admin
    g:traffic
)

SRCS(
    doc.go
    embed.go
)

GO_EMBED_PATTERN(vendor/*)

END()
