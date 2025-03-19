PY23_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(
    certificate_manager.py
    compute_metadata.py
    dbaas.py
    dns.py
    lockbox.py
    mdb_clickhouse.py
    mdb_firewall.py
    mdb_kafka.py
    mdb_network.py
    mdb_s3.py
    mdb_zookeeper.py
    mdb_postgresql.py
    mdb_redis.py
    mdb_mysql_users.py
    mdb_mysql.py
    mdb_resolv_conf.py
    mdb_elasticsearch.py
    mysql_hashes.py
    mdb_greenplum.py
)

PEERDIR(
    cloud/mdb/salt-tests/common
    contrib/python/bcrypt
    contrib/python/psycopg2
    contrib/python/six
    contrib/python/Jinja2
    contrib/python/requests
)

IF (PYTHON2)
    PEERDIR(
        contrib/deprecated/python/ipaddress
    )
ENDIF()

NEED_CHECK()

END()

RECURSE_ROOT_RELATIVE(
    cloud/mdb/salt-tests/modules
)
