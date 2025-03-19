PY23_LIBRARY()

STYLE_PYTHON()

OWNER(g:mdb)

PY_SRCS(
    dns.py
    fs.py
    mdb_clickhouse.py
    mdb_elasticsearch.py
    mdb_kafka.py
    mdb_kafka_connect.py
    mdb_mysql_schema.py
    mdb_mysql_users.py
    mdb_mysql.py
    mdb_postgresql.py
    mdb_redis.py
    mdb_s3.py
    mongodb.py
    mdb_mongodb.py
    mongodb_user.py
    porto_container.py
    porto_volume.py
    postgresql_cmd.py
    postgresql_schema.py
    zookeeper.py
)

PEERDIR(
    contrib/python/PyYAML
    contrib/python/pymongo
    contrib/python/kazoo
    contrib/python/six
    cloud/mdb/salt-tests/common
)

#NEED_REVIEW()
NEED_CHECK()

END()

RECURSE_ROOT_RELATIVE(
    cloud/mdb/salt-tests/states
)
