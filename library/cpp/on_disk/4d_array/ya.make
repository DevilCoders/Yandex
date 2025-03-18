LIBRARY()

OWNER(
    kostik
    g:base
)

SRCS(
    array4d_poly.cpp
    array4d_poly_footer.proto
    array4d_poly_wrapper.cpp
    memory4d_poly.cpp
)

PEERDIR(
    library/cpp/bit_io
)

END()
