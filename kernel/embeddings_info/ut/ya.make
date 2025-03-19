UNITTEST_FOR(kernel/embeddings_info)

OWNER(
    ulyanin
    defunator
)

SRCS(
    dssm_embeddings_ut.cpp
)

DATA(
    arcadia/kernel/embeddings_info/ut/compressed_user_rec_dssm_spy_title_domain_test.txt
    arcadia/kernel/embeddings_info/ut/compressed_url_rec_dssm_spy_title_domain_test.txt
    arcadia/kernel/embeddings_info/ut/uncompressed_cf_sharp_test.txt
)

END()
