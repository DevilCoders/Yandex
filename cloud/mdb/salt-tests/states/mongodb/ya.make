PY23_TEST()
OWNER(g:mdb)

PEERDIR(
        cloud/mdb/salt/salt/_states
        cloud/mdb/salt-tests/common
        contrib/python/mock
        contrib/python/PyYAML
        )

TEST_SRCS(
        mock_mongodb.py
        test_helpers.py
        is_alive.py
        sync_users.py
        sync_roles.py
        sync_roles_v2_userdb_roles.py
        mdb_mongodb_user_roles_for_db.py
        mdb_mongodb_user_roles.py
        mdb_mongodb_user.py
        mdb_mongodb_user_list.py
        mongodb_4_0_single.py
        mdb_mongodb_roles_list.py
        mdb_mongodb_role.py
        ensure_shard_zones.py
        ensure_check_collections.py
        ensure_balancer_state.py
        ensure_databases.py
        ensure_resetup_host.py
        ensure_stepdown_host.py
        ensure_member_in_replicaset.py
)

TIMEOUT(60)
SIZE(SMALL)
END()
