LIBRARY()

OWNER(udovichenko-r)

SRCS(
    phone.cpp
    phone_collect.cpp
    phone_proc.cpp
    check_phone_proc.cpp
    tel_schemes.cpp
    telephonoid.cpp
    simple_tokenizer.cpp
    text_telfinder.cpp
    telfinder.cpp
)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/containers/sorted_vector
    library/cpp/deprecated/iter
    library/cpp/solve_ambig
)

END()
