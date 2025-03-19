.PHONY: lint yapf isort flake8 pylint bandit test test_postgresql test_postgresql_11 test_postgresql_11_1c test_postgresql_single \
	test_postgresql_upgrade_11 test_mongodb test_mongodb_single test_mongodb_sharded \
	test_redis test_redis_single test_redis_sharded \
	test_redis_6 test_redis_6_single test_redis_6_sharded \
	test_redis_6_tls test_redis_6_tls_sharded test_redis_62_tls test_redis_62_tls_sharded \
	test_mysql test_mysql_8_0 test_mysql_single \
	test_clickhouse test_clickhouse_backup_service test_clickhouse_go_api test_clickhouse_keeper test_clickhouse_scale_upgrade \
	test_clickhouse_sql_management test_clickhouse_single test_clickhouse_single_scale_upgrade test_clickhouse_single_sql_management \
	test_clickhouse_single_cloud_storage test_clickhouse_single_to_ha \
	test_clickhouse_tls \
	test_deploy_v2 clean clean_pycache clean_venv clean_env_file \
	reset stop_containers create_env start_env start_arbiter stop_env format generate_proto help \
	test_greenplum
SESSION_FILE=.session_conf.sav
BEHAVE_CMD=env LANG=en_US.UTF-8 ./venv/bin/behave
BEHAVE_OPTS=--show-timings --tags=-wip --tags=-skip
ifdef junit-directory
BEHAVE_OPTS+= --junit --junit-directory=${junit-directory}
endif
TEST_ROOT=${PWD}
ARCADIA_ROOT=${PWD}/../../..
PROTOBUF_PATHS=cloud/bitbucket/common-api cloud/bitbucket/private-api contrib/libs/googleapis-common-protos \
               contrib/libs/protobuf/python/

venv:
	python3 -m venv venv
	./venv/bin/pip install --no-cache-dir --disable-pip-version-check \
			-r requirements.txt

lint: generate_proto yapf isort flake8 pylint unittest

yapf: venv
	./venv/bin/yapf -rpd images tests

isort: venv
	./venv/bin/isort --recursive --check-only --ignore-whitespace --diff images tests

flake8: venv
	./venv/bin/flake8 images tests

pylint: venv
	./venv/bin/pylint images tests

unittest: venv
	./venv/bin/python -m pytest unit_tests

test: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/internal_api_auth.feature

test_postgresql: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_cluster.feature

test_postgresql_1c: venv generate_proto
	exit 0

test_postgresql_11: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_cluster_11.feature

test_postgresql_11_1c: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_cluster_11_1c.feature

test_postgresql_12: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_cluster_12.feature

test_postgresql_12_1c: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_cluster_12_1c.feature

test_postgresql_13: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_cluster_13.feature

test_postgresql_13_1c: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_cluster_13_1c.feature

test_postgresql_14: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_cluster_14.feature

test_postgresql_14_1c: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_cluster_14_1c.feature

test_postgresql_single: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_single.feature

test_postgresql_upgrade_11: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_upgrade_11.feature

test_postgresql_upgrade_12: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_upgrade_12.feature

test_postgresql_upgrade_13: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_upgrade_13.feature

test_postgresql_upgrade_14: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_upgrade_14.feature

test_postgresql_upgrade_all_versions: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_upgrade_all_versions.feature

test_postgresql_upgrade_all_versions_1c: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/postgres_upgrade_all_versions_1c.feature

test_mongodb:
	exit 0

test_mongodb_upgrade_4_0:
	exit 0

test_mongodb_single:
	exit 0

test_mongodb_sharded:
	exit 0

test_mongodb_sharded_part2:
	exit 0

test_mongodb_4_0: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_0_cluster.feature

test_mongodb_4_0_single: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_0_single.feature

test_mongodb_4_0_sharded: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_0_sharded.feature

test_mongodb_4_0_sharded_part2: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_0_sharded_part2.feature

test_mongodb_4_2: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_2_cluster.feature

test_mongodb_4_2_single: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_2_single.feature

test_mongodb_4_2_sharded: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_2_sharded.feature

test_mongodb_4_2_sharded_part2: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_2_sharded_part2.feature

test_mongodb_upgrade_4_2: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_cluster_upgrade_4_2.feature

test_mongodb_sharded_upgrade_4_2: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_sharded_upgrade_4_2.feature

test_mongodb_4_4: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_4_cluster.feature

test_mongodb_4_4_single: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_4_single.feature

test_mongodb_4_4_sharded: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_4_sharded.feature

test_mongodb_4_4_sharded_part2: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_4_sharded_part2.feature

test_mongodb_upgrade_4_4: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_cluster_upgrade_4_4.feature

test_mongodb_sharded_upgrade_4_4: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_sharded_upgrade_4_4.feature

test_mongodb_4_4_enterprise: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_4_4_enterprise_cluster.feature

test_mongodb_5_0: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_5_0_cluster.feature

test_mongodb_5_0_single: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_5_0_single.feature

test_mongodb_5_0_sharded: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_5_0_sharded.feature

test_mongodb_5_0_sharded_part2: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_5_0_sharded_part2.feature

test_mongodb_upgrade_5_0: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_cluster_upgrade_5_0.feature

test_mongodb_sharded_upgrade_5_0: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_sharded_upgrade_5_0.feature

test_mongodb_5_0_enterprise: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mongodb_5_0_enterprise_cluster.feature

test_redis: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/redis_cluster.feature

test_redis_single: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/redis_single.feature

test_redis_sharded: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/redis_sharded.feature

test_redis_6: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/redis_6_cluster.feature

test_redis_62_single: venv generate_proto
	exit 0

test_redis_6_sharded: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/redis_6_sharded.feature

test_redis_62_tls: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/redis_62_tls_cluster.feature

test_redis_62_tls_sharded: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/redis_62_tls_sharded.feature

test_redis_7_cluster: venv generate_proto
	exit 0

test_redis_7_single: venv generate_proto
	exit 0

test_redis_7_sharded: venv generate_proto
	exit 0

test_redis_7_cluster_tls: venv generate_proto
	exit 0

test_redis_7_sharded_tls: venv generate_proto
	exit 0

test_mysql: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mysql_cluster.feature

test_mysql_8_0: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mysql_cluster_8_0.feature

test_mysql_single: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mysql_single.feature

test_mysql_upgrade_80: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/mysql_upgrade_80.feature

test_clickhouse: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_cluster.feature

test_clickhouse_backup_service: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_backup_service.feature

test_clickhouse_go_api: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_cluster_go_api.feature

test_clickhouse_keeper: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_cluster_keeper.feature

test_clickhouse_scale_upgrade: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_cluster_scale_upgrade.feature

test_clickhouse_sql_management: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_cluster_sql_management.feature

test_clickhouse_single: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_single.feature

test_clickhouse_single_scale_upgrade: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_single_scale_upgrade.feature

test_clickhouse_single_cloud_storage: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_single_cloud_storage.feature

test_clickhouse_single_sql_management: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_single_sql_management.feature

test_clickhouse_single_to_ha: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_single_to_ha.feature

test_clickhouse_tls: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/clickhouse_cluster_tls.feature

test_opensearch_single: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/opensearch_single.feature

test_opensearch_cluster: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/opensearch_cluster.feature

test_elasticsearch_7_10: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_cluster.feature -D version=7.10

test_elasticsearch_single_7_10: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_single.feature -D version=7.10

test_elasticsearch_7_11: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_cluster.feature -D version=7.11

test_elasticsearch_single_7_11: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_single.feature -D version=7.11

test_elasticsearch_7_12: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_cluster.feature -D version=7.12

test_elasticsearch_single_7_12: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_single.feature -D version=7.12

test_elasticsearch_7_13: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_cluster.feature -D version=7.13

test_elasticsearch_single_7_13: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_single.feature  -D version=7.13

test_elasticsearch_7_14: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_cluster.feature -D version=7.14

test_elasticsearch_single_7_14: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_single.feature  -D version=7.14

test_elasticsearch_7_15: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_cluster.feature -D version=7.15

test_elasticsearch_single_7_15: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_single.feature  -D version=7.15

test_elasticsearch_7_16: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_cluster.feature -D version=7.16

test_elasticsearch_single_7_16: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_single.feature  -D version=7.16

test_elasticsearch_7_17: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_cluster.feature -D version=7.17

test_elasticsearch_single_7_17: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/elasticsearch_single.feature  -D version=7.17

ifneq ("${ARCANUMID}", "trunk")
ES_TAGS=--tags=-upgrade
else
ES_TAGS=
endif

test_elasticsearch_scale_upgrade: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} ${ES_TAGS} tests/features/elasticsearch_cluster_scale_upgrade.feature

test_greenplum: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/greenplum_cluster.feature

test_deploy_v2: venv generate_proto
	${BEHAVE_CMD} ${BEHAVE_OPTS} tests/features/deploy_v2.feature

clean: stop_env clean_pycache clean_venv clean_env_file clean_proto
	rm -fr staging

clean_pycache:
	find . -name __pycache__ -type d -exec rm -rf {} +

clean_venv:
	rm -rf venv

clean_env_file:
	rm -f ${SESSION_FILE}

clean_proto:
	rm -rf yandex google cloud

reset: stop_env clean_pycache clean_env_file

stop_containers:
	docker ps | fgrep `./venv/bin/python -m tests.configuration` | awk '{print $$1}' | xargs docker stop || true

create_env: venv
	PATH=./venv/bin:$$PATH ./venv/bin/python -m tests.env_control create

start_env: create_env
	PATH=./venv/bin:$$PATH ./venv/bin/python -m tests.env_control start

start_arbiter: start_env
	PATH=./venv/bin:$$PATH ./venv/bin/python -m tests.env_control start_arbiter

stop_env: venv
	test -d ./venv/bin && test -f ${SESSION_FILE} && \
	PATH=./venv/bin:$$PATH ./venv/bin/python -m tests.env_control stop || true

format: venv generate_proto
	./venv/bin/isort --recursive --apply images tests unit_tests
	./venv/bin/yapf --recursive --parallel --in-place images tests unit_tests

generate_proto:
	${ARCADIA_ROOT}/ya make --checkout --add-protobuf-result --add-result=.py --output=../../../ --no-src-links python-protobuf
	for proto_path in ${PROTOBUF_PATHS}; do \
		cd ${ARCADIA_ROOT}/$$proto_path && \
		find . -name '*.py' | grep pb2 | cpio -pdm ${TEST_ROOT} ; \
	done;
	cd ${TEST_ROOT}
	mkdir -p cloud/mdb/mlock/api
	cp ${ARCADIA_ROOT}/cloud/mdb/mlock/api/*_pb2*.py cloud/mdb/mlock/api

help:
	@echo 'Common targets:'
	@echo '  lint                   Run linter tools on tests source code.'
	@echo '  test                   Run common for all dbs tests.'
	@echo '  create_env             Create test environment.'
	@echo '  start_env              Start test environment runtime.'
	@echo '  start_arbiter          Start test environment with arbiter. Runs until C-c is pressed.'
	@echo '  stop_env               Stop test environment runtime.'
	@echo '  clean                  Clean up test environment left from the previous test run.'
	@echo '  clean_pycache          Clean up __pycache__ directories.'
	@echo '  reset                  Reset the environment: alias for `make stop_env clean_pycache clean_env_file`.'
	@echo '                         Code remains intact'
	@echo '  stop_containers        Stop your containers.'
	@echo '  format                 Re-format source code to conform style settings enforced by'
	@echo '                         isort and yapf tools.'
	@echo '  help                   Show this help message.'
