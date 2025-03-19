.PHONY: \
	test \
	clean clean_pycache clean_venv clean_env_file \
	reset stop_containers create_env start_env stop_env format help
SESSION_FILE=.session_conf.sav
BEHAVE_OPTS=--show-timings --tags=-wip --tags=-skip --no-skipped

ifndef BASTION
BASTION2="lb.bastion.cloud.yandex.net"
endif
ifdef BASTION
BASTION2="${BASTION}"
endif
BASTION = $(BASTION2)
GO_PATH=$(shell go env GOPATH)

ifdef junit-directory
BEHAVE_OPTS+= --junit --junit-directory=${junit-directory}
endif


ifneq ($(ARCANUMID),trunk)
BEHAVE_OPTS+= --stop --tags=-trunk
endif

ifndef BEHAVE_TARGET
BEHAVE_TARGET=@tests/full.featureset
endif

USER ?= $(shell whoami)

# robot-pgaas-ci runs on jenkins without bastion
ifeq ($(USER),robot-pgaas-ci)
BASTION=
endif
ifdef DISABLE_BASTION
BASTION=
endif

.PHONY: venv
venv:
	@echo "Skipping venv"

ifndef Y_PYTHON_SOURCE_ROOT
.PHONY: infratests
endif
infratests: staging/logs staging/bin/yc
	@echo "Building behave to ./infratests"
	ya make

lint:
	ya make -t

unittest:
	@echo "Implement ya make -t"

staging:
	mkdir -p staging

staging/logs: staging
	mkdir -p staging/logs

short_test: infratests
	BASTION=$(BASTION) ./infratests ${BEHAVE_OPTS} @tests/fast.featureset

yandex:
	@echo "Skipping ./venv/bin/python -m tests.generate_grpc_specs"

.PHONY: start_kafka_env
start_kafka_env: infratests
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main PRODUCT=kafka ./infratests start_managed

kafka: infratests
	BASTION=$(BASTION) DISABLE_INIT=true ./infratests ${BEHAVE_OPTS} @tests/kafka.featureset

.PHONY: start_sqlserver_env
start_sqlserver_env: infratests
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main PRODUCT=sqlserver ./infratests start_managed

sqlserver: infratests
	BASTION=$(BASTION) PRODUCT=sqlserver DISABLE_INIT=true ./infratests ${BEHAVE_OPTS} @tests/featuresets/sqlserver.featureset

sqlserver_enterprise_multi: infratests
	BASTION=$(BASTION) PRODUCT=sqlserver DISABLE_INIT=true ./infratests ${BEHAVE_OPTS} @tests/featuresets/sqlserver_enterprise_multi.featureset

sqlserver_enterprise_single: infratests
	BASTION=$(BASTION) PRODUCT=sqlserver DISABLE_INIT=true ./infratests ${BEHAVE_OPTS} @tests/featuresets/sqlserver_enterprise_single.featureset

sqlserver_standard_multi: infratests
	BASTION=$(BASTION) PRODUCT=sqlserver DISABLE_INIT=true ./infratests ${BEHAVE_OPTS} @tests/featuresets/sqlserver_standard_multi.featureset

sqlserver_standard_single: infratests
	BASTION=$(BASTION) PRODUCT=sqlserver DISABLE_INIT=true ./infratests ${BEHAVE_OPTS} @tests/featuresets/sqlserver_standard_single.featureset

.PHONY: start_greenplum_env
start_greenplum_env: infratests
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main PRODUCT=greenplum ./infratests start_managed

greenplum: infratests
	BASTION=$(BASTION) DISABLE_INIT=true ./infratests ${BEHAVE_OPTS} @tests/greenplum.featureset

.PHONY: start_dataproc_env
start_dataproc_env: infratests
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main PRODUCT=dataproc ./infratests start

dataproc: staging infratests automatic-gc
	BASTION=$(BASTION) ./infratests ${BEHAVE_OPTS} @tests/full.featureset

.PHONY: start_redis_env
start_redis_env: infratests
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main PRODUCT=redis ./infratests start_managed

redis: infratests
	BASTION=$(BASTION) DISABLE_INIT=true ./infratests ${BEHAVE_OPTS} @tests/redis.featureset

test: staging infratests automatic-gc
	BASTION=$(BASTION) ./infratests ${BEHAVE_OPTS} ${BEHAVE_TARGET}

clean: clean_env clean_pycache clean_venv clean_env_file
	rm -fr staging

clean_env_file:
	rm -f ${SESSION_FILE}

.PHONY: clean_secrets
clean_secrets:
	rm -rf staging/code/salt/srv/pillar/compute/preprod
	rm -rf staging/code/salt/srv/pillar/mdb_controlplane_compute_preprod

.PHONY: reset
reset: stop_env clean_pycache clean_env_file

.PHONY: cache
cache: infratests
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main ./infratests cache

.PHONY: start_env
start_env: start_dataproc_env

.PHONY: start_managed
start_managed: infratests
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main ./infratests start_managed

.PHONY: stop_env
stop_env: infratests
	test -f ${SESSION_FILE} && \
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main ./infratests stop || true

.PHONY: clean_env
clean_env: infratests
	test -f ${SESSION_FILE} && \
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main ./infratests clean || true

.PHONY: watch_job_output
watch_job_output: infratests
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main ./infratests watch_job_output

.PHONY: ssh_to_master
ssh_to_master: infratests
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main ./infratests ssh_to_master

.PHONY: gc
gc: infratests
	BASTION=$(BASTION) PATH=./venv/bin:$$PATH DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main ./infratests gc_infratest_resources

.PHONY: automatic-gc
ifeq ($(USER),robot-pgaas-ci)
automatic-gc: infratests
	@echo "Automatic garbage collection"
	make gc
else
automatic-gc:
	@echo "Skipping automatic gc, because you are not robot-pgaas-ci"
endif

.PHONY: list_infratest_vms
list_infratest_vms: infratests
	BASTION=$(BASTION) DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main ./infratests list_infratest_vms

.PHONY: sync_code
sync_code: infratests
	BASTION=$(BASTION) DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main ./infratests sync_code

.PHONY: sync_salt
sync_salt: infratests
	BASTION=$(BASTION) DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main ./infratests sync_salt

.PHONY: sync_salt_managed
sync_salt_managed: infratests
	BASTION=$(BASTION) DISABLE_INIT=false Y_PYTHON_ENTRY_POINT=tests.env_control:cli_main ./infratests sync_salt_managed

.PHONY: sync
sync: sync_code

~/go/src/bb.yandex-team.ru/cloud/cloud-go:
	git clone --depth 1 ssh://git@bb.yandex-team.ru/cloud/cloud-go.git "$(GO_PATH)/src/bb.yandex-team.ru/cloud/cloud-go"

staging/bin/yc: staging
	wget https://s3.mds.yandex.net/mcdev/internal-cli/install.sh -q -O staging/yc_install.sh
	chmod +x staging/yc_install.sh
	bash ./staging/yc_install.sh -n -i ./staging/

.PHONY: build_yc
build_yc: ~/go/src/bb.yandex-team.ru/cloud/cloud-go staging
	git --git-dir="$(GO_PATH)/src/bb.yandex-team.ru/cloud/cloud-go/.git" \
        --work-tree="$(GO_PATH)/src/bb.yandex-team.ru/cloud/cloud-go" \
        checkout
	go build -o "./staging/bin/yc" bb.yandex-team.ru/cloud/cloud-go/cli/cmd/yc

.PHONY: shell
shell: infratests
	Y_PYTHON_ENTRY_POINT=tests.shell:infratest_shell ./infratests

help:
	@echo 'Common targets:'
	@echo '  staging/bin/yc         Install production YC CLI'
	@echo '  build_yc               Build YC CLI from sources'
	@echo '  test                   Run common for all dbs tests.'
	@echo '  short_test             Run only create and remove test.'
	@echo '  start_env              Start test environment runtime.'
	@echo '  stop_env               Stop test environment runtime.'
	@echo '  clean_env              Delete test environment runtime.'
	@echo '  clean                  Clean up test environment left from the previous test run.'
	@echo '  clean_pycache          Clean up __pycache__ directories.'
	@echo '  reset                  Reset the environment: alias for `make stop_env clean_pycache clean_env_file`.'
	@echo '                         Code remains intact'
	@echo '  stop_containers        Stop your containers.'
	@echo '  format                 Re-format source code to conform style settings enforced by'
	@echo '                         isort and yapf tools.'
	@echo '  help                   Show this help message.'
