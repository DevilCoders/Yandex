PY3_LIBRARY(bootstrap.db.admin)

OWNER(g:ycselfhost)

PEERDIR(
    library/python/resource
    library/python/retry


    contrib/python/psycopg2
    contrib/python/PyYAML
    contrib/python/schematics

    cloud/bootstrap/common
)

RESOURCE(
    cloud/bootstrap/db/data/local/docker-compose.yml docker-compose-local-postgres.yml
    cloud/bootstrap/db/data/local/localdb.yaml localdb.yaml
    cloud/bootstrap/db/schema/current/bootstrap.sql current.bootstrap.sql
)

PY_SRCS(
    NAMESPACE bootstrap.db.admin

    app.py
)

END()
