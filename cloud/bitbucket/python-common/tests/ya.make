PY3TEST()

OWNER(zasimov-a)

SIZE(SMALL)

TEST_SRCS(
    conftest.py
    contrail/__init__.py
    contrail/test_path.py
    contrail/test_resource.py
    contrail/test_schema.py
    __init__.py
    test_allocator.py
    test_api_client.py
    test_arcadia.py
    test_backoff.py
    test_config.py
    test_context.py
    test_fields.py
    test_formatting.py
    test_grpc_base_model.py
    test_handling.py
    test_ids.py
    test_juggler_cllient.py
    test_kikimr_client.py
    test_l3manager_client.py
    test_labels.py
    test_metrics.py
    test_misc.py
    test_models.py
    test_network_client.py
    test_nlb.py
    test_paging.py
    test_resources.py
    test_sql.py
    test_throttler.py
    test_tskv.py
    test_validation.py
    utils.py
)

PEERDIR(
    cloud/bitbucket/python-common
    contrib/python/pytest
)

DATA(
    arcadia/cloud/bitbucket/python-common/tests/l3manager
)

RESOURCE(
    arcadia/file.txt arcadia/file.txt
    arcadia/folder/another_file.txt arcadia/folder/another_file.txt
)

END()
