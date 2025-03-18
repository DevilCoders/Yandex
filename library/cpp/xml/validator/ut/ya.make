UNITTEST_FOR(library/cpp/xml/validator)

OWNER(zhigan)

PEERDIR(
    library/cpp/xml/document
)

DATA(arcadia/library/cpp/xml/validator/test_data)

SRCS(
    validator_ut.cpp
)

END()
