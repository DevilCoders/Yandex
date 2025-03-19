PY23_TEST()
OWNER(g:mdb)

PEERDIR(
    cloud/mdb/salt/salt/_modules
    cloud/mdb/salt-tests/common
    cloud/mdb/internal/python/pytest
    contrib/python/mock
)

TEST_SRCS(
    test_backup_config.py
    test_calculated_attributes.py
    test_render_cluster_config.py
    test_render_dictionary_config.py
    test_render_kafka_config.py
    test_render_ml_model_config.py
    test_render_raft_config.py
    test_render_server_resetup_config.py
    test_render_storage_config.py
    test_render_users_config.py
    test_resetup_required.py
    test_restore_user_object_cache.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
