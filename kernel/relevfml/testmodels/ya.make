LIBRARY()

OWNER(
    apos
)

FROM_SANDBOX(FILE 1098341867 OUT_NOAUTO mn464170.info)

ARCHIVE_ASM(
    NAME TestModels
    DONTCOMPRESS
    ip25.info
    Ru.info
    RuFreshExtRelev.info
    quark_slices.info
    ru_hast.info
    ru_fast.info
    FlatbufSingleDataModel.info
    mn464170.info
)

END()
