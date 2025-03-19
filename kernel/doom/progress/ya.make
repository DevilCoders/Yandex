LIBRARY()

OWNER(
    elric
    mvel
    g:base
)

PEERDIR(
    library/cpp/progress_bar
)

SRCS(
    abstract_progress_callback.cpp
    null_progress_callback.cpp
    progress.cpp
    progress_callback_factory.cpp
    stream_progress_callback.cpp
)

END()
