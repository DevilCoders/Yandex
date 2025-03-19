UNITTEST()

OWNER(
    dmitryno
    g:wizard
    gotmanov
)

PEERDIR(
    ADDINCL kernel/gazetteer
    library/cpp/archive
    kernel/gazetteer/simpletext
)

SRCDIR(kernel/gazetteer)

ARCHIVE(
    NAME test_gazetteer.inc
    test_compile.gzt
    test_compile.gztproto
    test_geo.gztproto
    test_import.gzt
    test_import1.gztproto
    test_import2.gztproto
    test_builtins.gzt
    test_norm.gzt
    #   test_builtins.gztproto - intentionally not included here (accessed only via generated_pool)
    test_not_compile1.gzt
    test_not_compile2.gzt
    test_not_compile3.gzt
    test_morphology.gzt
    test_filters.gzt
    test_tokenize.gzt
    test_diacritics1.gzt
    test_diacritics2.gzt
)

SRCS(
    gazetteer_ut.cpp
    test_import1.gztproto
    test_import2.gztproto
    test_compile.gztproto
    test_builtins.gztproto
    test_geo.gztproto
)

END()
