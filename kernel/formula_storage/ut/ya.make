OWNER(nkmakarov)

UNITTEST_FOR(kernel/formula_storage)

PEERDIR(
    kernel/extended_mx_calcer/calcers
    library/cpp/resource
)

SRCS(
    formula_storage_ut.cpp
)

RESOURCE(
    ${ARCADIA_ROOT}/kernel/formula_storage/ut_data/info.info info
    ${ARCADIA_ROOT}/kernel/formula_storage/ut_data/from_fml.info from_fml
    ${ARCADIA_ROOT}/kernel/formula_storage/ut_data/catboost_info.cbm catboost_info
    ${ARCADIA_ROOT}/kernel/formula_storage/ut_data/mnmc.mnmc mnmc
    ${ARCADIA_ROOT}/kernel/formula_storage/ut_data/regtree.regtree regtree
    ${ARCADIA_ROOT}/kernel/formula_storage/ut_data/models.archive models
    ${ARCADIA_ROOT}/kernel/formula_storage/ut_data/with_mnmc.archive archive_with_mnmc
    ${ARCADIA_ROOT}/kernel/formula_storage/ut_data/xtd.xtd xtd
)

REQUIREMENTS(network:full)

END()
