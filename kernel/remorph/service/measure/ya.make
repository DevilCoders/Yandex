LIBRARY()

OWNER(g:remorph)

PEERDIR(
    kernel/gazetteer
    kernel/remorph/facts
    library/cpp/containers/sorted_vector
)

SRCS(
    measure.cpp
)

END()
