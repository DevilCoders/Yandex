PY3TEST()

OWNER(g:ycselfhost)

INCLUDE(${ARCADIA_ROOT}/library/recipes/docker_compose/recipe.inc)

SIZE(LARGE)

TAG(
    ya:external
    ya:fat
    ya:force_sandbox
    ya:nofuse
)

REQUIREMENTS(
    # container:716524173 bionic
    container:799092394 # xenial
    cpu:all dns:dns64
)

PEERDIR(
    library/python/resource
    library/python/retry

    contrib/python/flask-restplus
    contrib/python/psycopg2
    contrib/python/PyYAML
    contrib/python/schematics

    cloud/bootstrap/common
    cloud/bootstrap/api/src/backend
    cloud/bootstrap/db/src/admin
)


DEPENDS(
    cloud/bootstrap/api/bin/backend
)

RESOURCE(
    cloud/bootstrap/api/bin/backend/localdb.yaml localapi.yaml
)

TEST_SRCS(
    conftest.py
    common.py

    test_admin.py
    test_migration.py

    # routes tests
    routes/test_admin.py
    routes/test_auth.py
    routes/test_cluster_maps.py
    routes/test_hosts_and_svms.py
    routes/test_instance_groups.py
    routes/test_instance_salt_roles.py
    routes/test_locks.py
    routes/test_salt_roles.py
    routes/test_stands.py
)


END()
