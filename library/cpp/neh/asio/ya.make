OWNER(
    and42 # former neh maintainer
    petrk
    pg
    rumvadim # current neh maintainer
    g:middle
)

LIBRARY()

PEERDIR(
    contrib/libs/libc_compat
    library/cpp/coroutine/engine
    library/cpp/dns
    library/cpp/deprecated/atomic
)

SRCS(
    asio.cpp
    deadline_timer_impl.cpp
    executor.cpp
    io_service_impl.cpp
    poll_interrupter.cpp
    tcp_acceptor_impl.cpp
    tcp_socket_impl.cpp
)

END()
