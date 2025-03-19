.PHONY: clean all

GP_MAJOR=6
GP_VERSION=6.19.3-10-yandex.52831.04ebd7f7e6
PROJECT=gpsync
ZK_VERSION=3.7.1
export ZK_VERSION

ARCADIA_ROOT=../../..
YA_BIN=$(ARCADIA_ROOT)/ya
YA_BUILD_DIR=ya_build

clean_report:
	rm -rf htmlcov

clean: clean_report
	rm -rf ../yamail-gpsync_*.build ../yamail-gpsync_*.changes ../yamail-gpsync_*.deb Dockerfile* docker/zookeeper/zookeeper-*.tar.gz test_ssh_key*
	mv --force static/gpsync.sudoers.d.orig static/gpsync.sudoers.d 2>/dev/null || true
	mv --force static/gpsync.init.d.orig static/gpsync.init.d 2>/dev/null || true
	rm -rf .tox __pycache__ gpsync.egg-info .mypy_cache
	rm -rf junit_report

$(YA_BUILD_DIR)/gpsync: gpsync
	mkdir -p $(YA_BUILD_DIR) && cp gpsync $(YA_BUILD_DIR)/gpsync

$(YA_BUILD_DIR)/gpsync-util: gpsync-util/gpsync-util
	mkdir -p $(YA_BUILD_DIR) && cp gpsync-util/gpsync-util $(YA_BUILD_DIR)/gpsync-util

gpsync: __init__.py cli.py command_manager.py exceptions.py failover_election.py helpers.py main.py pg.py plugin.py plugins/pgbouncer.py plugins/upload_wals.py replication_manager.py sdnotify.py utils.py zk.py ya.make
	$(YA_BIN) make --target-platform=default-linux-x86_64 .

gpsync-util/gpsync-util: gpsync gpsync-util/ya.make
	$(YA_BIN) make --target-platform=default-linux-x86_64 gpsync-util

tests/behave: tests/environment.py tests/steps/cluster.py tests/steps/config.py tests/steps/database.py tests/steps/zk.py tests/steps/helpers.py tests/ya.make
	$(YA_BIN) make tests

build: $(YA_BUILD_DIR)/gpsync $(YA_BUILD_DIR)/gpsync-util
	yes | ssh-keygen -m PEM -t rsa -N '' -f test_ssh_key -C gpadmin || true
	wget https://mirror.yandex.ru/mirrors/apache/zookeeper/zookeeper-$(ZK_VERSION)/apache-zookeeper-$(ZK_VERSION)-bin.tar.gz -nc -O docker/zookeeper/zookeeper-$(ZK_VERSION).tar.gz || true
	docker-compose -p $(PROJECT) down --rmi all --remove-orphans
	docker-compose -p $(PROJECT) build --build-arg gp_major=$(GP_MAJOR) --build-arg gp_version=$(GP_VERSION) --build-arg ZK_VERSION=$(ZK_VERSION)
	docker-compose -p $(PROJECT) up -d
	timeout 600 docker exec gpsync_greenplum_master1_1 /bin/su - gpadmin /home/gpadmin/initcluster.sh
	docker commit gpsync_greenplum_master1_1 gpsync_greenplum_master1:latest
	docker commit gpsync_greenplum_master2_1 gpsync_greenplum_master2:latest
	docker commit gpsync_greenplum_segment1_1 gpsync_greenplum_segment1:latest
	docker commit gpsync_greenplum_segment2_1 gpsync_greenplum_segment2:latest
	docker-compose -p $(PROJECT) down


jepsen_test:
	# test has to be rewritten for Greenplum from scratch
	# until then, it is disabled

BEHAVE_ARGS=--logging-filter='-kazoo.client' --show-timings @cloud/mdb/gpsync/tests/gpsync.featureset --junit --junit-directory cloud/mdb/gpsync/junit_report
check_test: build tests/behave
	cd $(ARCADIA_ROOT) && \
	PROJECT=$(PROJECT) \
	GP_MAJOR=$(GP_MAJOR) \
	cloud/mdb/gpsync/tests/behave $(BEHAVE_ARGS) --stop $(TEST_ARGS)

check_test_unstoppable: build tests/behave
	cd $(ARCADIA_ROOT) && \
	PROJECT=$(PROJECT) \
	GP_MAJOR=$(GP_MAJOR) \
	cloud/mdb/gpsync/tests/behave $(BEHAVE_ARGS) $(TEST_ARGS)

lint:
	tox -e yapf,flake8,pylint,bandit

jepsen: build jepsen_test

check: build check_test

check_unstoppable: build check_test_unstoppable

check-world: clean build check_test jepsen_test
