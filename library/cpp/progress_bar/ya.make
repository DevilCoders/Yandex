LIBRARY()

OWNER(avitella)

PEERDIR(
    library/cpp/colorizer
    library/cpp/threading/future
)

SRCS(
    async_progress_bar.cpp
    builder.cpp
    data.cpp
    progress.cpp
    progress_bar.cpp
    simple_progress_bar.cpp
)

END()
