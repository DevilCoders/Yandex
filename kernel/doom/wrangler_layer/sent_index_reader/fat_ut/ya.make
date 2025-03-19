UNITTEST_FOR(kernel/doom/wrangler_layer/sent_index_reader)

OWNER(
    g:base
)

SRCS(
    sent_reader_ut.cpp
)

PEERDIR(
    kernel/sent_lens
    kernel/doom/wad
    ysite/yandex/erf
    kernel/sent_lens
    kernel/doom/progress

)

SIZE(LARGE)

TAG(ya:fat)

DEPENDS(
   kernel/doom/wrangler_layer/sent_index_reader/old_sent
)




END()
