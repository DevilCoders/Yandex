.PHONY: clean all

PG_MAJOR=14
PG_VERSION=14.0-201-yandex.1.70c4e0d
PGSYNC_IMAGE=pgsync:behave
PROJECT=pgsync
ZK_VERSION=3.7.1
export ZK_VERSION
REPLICATION_TYPE=quorum

ARCADIA_ROOT=../../..
YA_BIN=$(ARCADIA_ROOT)/ya
YA_BUILD_DIR=ya_build

clean_report:
	rm -rf htmlcov

clean: clean_report
	rm -rf ../yamail-pgsync_*.build ../yamail-pgsync_*.changes ../yamail-pgsync_*.deb Dockerfile* docker/zookeeper/zookeeper-*.tar.gz test_ssh_key*
	mv --force static/pgsync.sudoers.d.orig static/pgsync.sudoers.d 2>/dev/null || true
	mv --force static/pgsync.init.d.orig static/pgsync.init.d 2>/dev/null || true
	rm -rf .tox __pycache__ pgsync.egg-info .mypy_cache
	rm -rf junit_report

$(YA_BUILD_DIR)/pgsync: pgsync
	mkdir -p $(YA_BUILD_DIR) && cp pgsync $(YA_BUILD_DIR)/pgsync

$(YA_BUILD_DIR)/pgsync-util: pgsync-util/pgsync-util
	mkdir -p $(YA_BUILD_DIR) && cp pgsync-util/pgsync-util $(YA_BUILD_DIR)/pgsync-util

$(YA_BUILD_DIR)/wd_pgsync: wd_pgsync/wd_pgsync
	mkdir -p $(YA_BUILD_DIR) && cp wd_pgsync/wd_pgsync $(YA_BUILD_DIR)/wd_pgsync

pgsync: __init__.py cli.py command_manager.py exceptions.py failover_election.py helpers.py main.py pg.py plugin.py plugins/pgbouncer.py plugins/upload_wals.py replication_manager.py sdnotify.py utils.py zk.py ya.make
	$(YA_BIN) make --target-platform=default-linux-x86_64 .

pgsync-util/pgsync-util: pgsync pgsync-util/ya.make
	$(YA_BIN) make --target-platform=default-linux-x86_64 pgsync-util

wd_pgsync/wd_pgsync: pgsync wd_pgsync/wd_pgsync.py wd_pgsync/ya.make
	$(YA_BIN) make --target-platform=default-linux-x86_64 wd_pgsync

tests/behave: tests/environment.py tests/steps/cluster.py tests/steps/config.py tests/steps/database.py tests/steps/zk.py tests/steps/helpers.py tests/ya.make
	$(YA_BIN) make tests

build: $(YA_BUILD_DIR)/pgsync $(YA_BUILD_DIR)/pgsync-util $(YA_BUILD_DIR)/wd_pgsync
	cp -f docker/base/Dockerfile .
	yes | ssh-keygen -m PEM -t rsa -N '' -f test_ssh_key -C jepsen || true
	wget https://mirror.yandex.ru/mirrors/apache/zookeeper/zookeeper-$(ZK_VERSION)/apache-zookeeper-$(ZK_VERSION)-bin.tar.gz -nc -O docker/zookeeper/zookeeper-$(ZK_VERSION).tar.gz || true
	docker-compose -p $(PROJECT) down --rmi all --remove-orphans
	docker-compose -p $(PROJECT) -f jepsen-compose.yml down --rmi all --remove-orphans
	docker build -t pgsyncbase:latest .
	docker-compose -p $(PROJECT) build --build-arg replication_type=$(REPLICATION_TYPE) --build-arg pg_major=$(PG_MAJOR) --build-arg pg_version=$(PG_VERSION)

build_pgsync: $(YA_BUILD_DIR)/pgsync $(YA_BUILD_DIR)/pgsync-util $(YA_BUILD_DIR)/wd_pgsync
	rm -rf logs/
	cp -f tests/Dockerfile ./Dockerfile_pgsync_behave
	docker build -t $(PGSYNC_IMAGE) \
		--build-arg pg_major=$(PG_MAJOR) \
		-f ./Dockerfile_pgsync_behave .

jepsen_test:
	docker-compose -p $(PROJECT) -f jepsen-compose.yml up -d
	timeout 600 docker exec pgsync_postgresql1_1 setup.sh $(PG_MAJOR)
	timeout 600 docker exec pgsync_postgresql2_1 setup.sh $(PG_MAJOR) pgsync_postgresql1_1.pgsync_pgsync_net
	timeout 600 docker exec pgsync_postgresql3_1 setup.sh $(PG_MAJOR) pgsync_postgresql1_1.pgsync_pgsync_net
	mkdir -p logs
	(docker exec pgsync_jepsen_1 /root/jepsen/run.sh >logs/jepsen.log 2>&1 && tail -n 4 logs/jepsen.log) || ./docker/jepsen/save_logs.sh
	docker-compose -p $(PROJECT) -f jepsen-compose.yml down --rmi all

BEHAVE_ARGS=--logging-filter='-kazoo.client' --show-timings @cloud/mdb/pgsync/tests/pgsync.featureset --junit --junit-directory cloud/mdb/pgsync/junit_report
check_test: build_pgsync tests/behave
	cd $(ARCADIA_ROOT) && \
	PROJECT=$(PROJECT) \
	PGSYNC_IMAGE=$(PGSYNC_IMAGE) \
	PG_MAJOR=$(PG_MAJOR) \
	cloud/mdb/pgsync/tests/behave $(BEHAVE_ARGS) --stop $(TEST_ARGS)

check_test_unstoppable: build_pgsync tests/behave
	cd $(ARCADIA_ROOT) && \
	PROJECT=$(PROJECT) \
	PGSYNC_IMAGE=$(PGSYNC_IMAGE) \
	PG_MAJOR=$(PG_MAJOR) \
	cloud/mdb/pgsync/tests/behave $(BEHAVE_ARGS) $(TEST_ARGS)

lint:
	tox -e yapf,flake8,pylint,bandit

jepsen: build jepsen_test

check: build check_test

check_unstoppable: build check_test_unstoppable

check-world: clean build check_test jepsen_test
