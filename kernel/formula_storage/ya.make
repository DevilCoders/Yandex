LIBRARY()

OWNER(nkmakarov)

SRCS(
    formula_storage.cpp
)

PEERDIR(
    kernel/catboost
    kernel/ethos/lib/reg_tree
    kernel/formula_storage/shared_formulas_adapter
    kernel/matrixnet
    kernel/relevfml/models_archive
    kernel/extended_mx_calcer/factory
    kernel/index_mapping
    kernel/parallel_mx_calcer
    library/cpp/digest/md5
    library/cpp/archive
)

END()
