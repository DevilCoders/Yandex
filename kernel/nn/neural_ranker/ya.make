RECURSE(
    extract_from_factor_storage
    concat_features
    concat_features/ut
    protos
)

IF (OS_LINUX)
    RECURSE(
        extract_from_factor_storage/ut
        tf_apply_for_search
        tf_apply_for_search/ut
        tf_ops_for_search
    )
ENDIF()
