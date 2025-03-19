OWNER(akhropov)

PY2TEST()

TIMEOUT(240)

SIZE(MEDIUM)

TEST_SRCS(
    regions_db_test.py
    check_is_from_yandex_test.py
    user_type_resolver_test.py
    relev_region_resolver_test.py
    geo_helper_test.py
    inc_exc_region_resolver_test.py
    ipreg_ranges_test.py
    is_from_parent_region_trdb_test.py
    is_from_parent_region_trrr_test.py
)

DATA(
    sbr://3253110224  # 'geodata5.bin'@STABLE; 2022-06-27
    sbr://138589486 # ipreg_ranges_test.txt
)

DEPENDS(kernel/geo/tests/geo_utils)



END()
