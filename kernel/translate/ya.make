LIBRARY()

OWNER(mihaild)

SRCS(translate.cpp)

PEERDIR(
    kernel/lemmer/core
    kernel/lemmer/new_dict/eng
    kernel/lemmer/new_dict/rus
    kernel/lemmer/new_dict/tur
    kernel/lemmer/new_dict/ukr
    kernel/translate/generated
    library/cpp/langs
)

END()
