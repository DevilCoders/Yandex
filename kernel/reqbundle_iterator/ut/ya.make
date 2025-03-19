UNITTEST()

OWNER(
    g:base
    gotmanov
    grechnik
)

PEERDIR(
    kernel/doom/standard_models_storage
    kernel/indexann/protos
    kernel/indexann_data
    kernel/keyinv/invkeypos
    kernel/reqbundle_iterator
    kernel/xref
    ysite/yandex/indexann/reader
)

SRCS(
    reqbundle_iterator_ut.cpp
    constraint_checker_ut.cpp
)

END()
