PY3MODULE(bindings)

OWNER(
    g:mstand-online
)

SRCS(
    quality/functionality/turbo/urls_lib/python/lib/bindings.pyx
)

PEERDIR(
    quality/functionality/turbo/urls_lib/cpp/lib
    quality/functionality/turbo/urls_lib/cpp/canonize_lib
)

END()
