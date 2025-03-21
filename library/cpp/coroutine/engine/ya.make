LIBRARY()

OWNER(
    pg
    g:balancer
)

GENERATE_ENUM_SERIALIZATION(poller.h)

GENERATE_ENUM_SERIALIZATION(stack/stack_common.h)

PEERDIR(
    contrib/libs/libc_compat
    library/cpp/containers/intrusive_rb_tree
    library/cpp/deprecated/atomic
)

SRCS(
    cont_poller.cpp
    helper.cpp
    impl.cpp
    iostatus.cpp
    network.cpp
    poller.cpp
    sockpool.cpp
    stack/stack.cpp
    stack/stack_allocator.cpp
    stack/stack_guards.cpp
    stack/stack_storage.cpp
    stack/stack_utils.cpp
    trampoline.cpp
)

END()

RECURSE(
    stack/benchmark
    stack/ut
    ut
)
