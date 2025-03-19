LIBRARY()

OWNER(
    otrok
    g:facts
)

SRCS(
    voice_text.cpp
)

PEERDIR(
    kernel/lemmer/core
    library/cpp/deprecated/iter
    library/cpp/resource
    library/cpp/scheme
    library/cpp/telfinder
    library/cpp/token
    library/cpp/tokenizer
)

FROM_SANDBOX(FILE 220493491 OUT_NOAUTO phone_schemes.txt)
RESOURCE(phone_schemes.txt phone_schemes)

END()
