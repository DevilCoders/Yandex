#!/usr/bin/env make

REPO=a.yandex-team.ru/cloud/compute/snapshot
NAME=yc-snapshot
BUILDDT=$(shell date -u +%F@%H:%M:%S)
VERSION=$(shell git show-ref --head --hash head)
TAG=$(shell git describe --tags --always)
DEBVER=$(shell dpkg-parsechangelog | sed -n -e 's/^Version: //p')
LDFLAGS=-ldflags "-X ${REPO}/pkg/version.GitTag=${TAG} -X ${REPO}/pkg/version.Version=${DEBVER} -X ${REPO}/pkg/version.Build=${BUILDDT} -X ${REPO}/pkg/version.GitHash=${VERSION}"
TEST_CONFIG_PATH=$(shell pwd)/config.toml
TEST_TMPDIR=$(shell pwd)/tmpdir
PIDFILE=$(shell pwd)/yc-snapshot.pid
RESULTFILE=$(shell pwd)/result.txt

PROTOC_GEN_GRPC_GATEWAY=${GOPATH}/bin/protoc-gen-grpc-gateway
PROTOC_GEN_SWAGGER=${GOPATH}/bin/protoc-gen-swagger
PROTOC_GEN_GOGOFASTER=${GOPATH}/bin/protoc-gen-gogofaster
GOTESTTC=${GOPATH}/bin/go-test-teamcity

export PATH := $(PATH):/usr/lib/go-1.10/bin
SHELL := env PATH=$(PATH) /bin/bash

.DEFAULT: all
.PHONY: fmt vet test build

PKGS := $(shell go list ./... | grep -v vendor)

all: fmt vet test

vet:
	@echo "+ $@"
	@go vet $(PKGS)

fmt:
	@echo "+ $@"
	@test -z "$$(gofmt -s -l . 2>&1 | grep -v ^vendor/ | tee /dev/stderr)" || \
		(echo >&2 "+ please format Go code with 'gofmt -s'" && false)

test:
	@echo "+ $@"
	@echo $(PKGS)
	TEST_CONFIG_PATH=$(TEST_CONFIG_PATH) ./run_tests.sh $(PKGS)

build:
	@echo "+ $@"
	go build ${LDFLAGS} -o ${NAME} ${REPO}/cmd/snapshot
	go build ${LDFLAGS} -o ${NAME}-check ${REPO}/cmd/snapshot-check
	go build ${LDFLAGS} -o ${NAME}-download ${REPO}/cmd/snapshot-download
	go build ${LDFLAGS} -o ${NAME}-gc ${REPO}/cmd/gc
	go build ${LDFLAGS} -o ${NAME}-populate-database ${REPO}/tools/populate-database


grpc-generators: $(PROTOC_GEN_GRPC_GATEWAY) $(PROTOC_GEN_SWAGGER) $(PROTOC_GEN_GOGOFASTER)
	@echo "+ $@"

$(PROTOC_GEN_GOGOFASTER):
	@echo "+ $@"
	go install ./vendor/github.com/gogo/protobuf/protoc-gen-gogofaster

$(PROTOC_GEN_GRPC_GATEWAY):
	@echo "+ $@"
	go install ./vendor/github.com/grpc-ecosystem/grpc-gateway/protoc-gen-grpc-gateway

$(PROTOC_GEN_SWAGGER):
	@echo "+ $@"
	go install ./vendor/github.com/grpc-ecosystem/grpc-gateway/protoc-gen-swagger

grpc-go: grpc-generators
	cpp -P -traditional-cpp -nostdinc snapshot/snapshot_rpc/snapshot_raw.proto snapshot/snapshot_rpc/snapshot.proto
	cpp -DSWAGGER -P -traditional-cpp -nostdinc snapshot/snapshot_rpc/snapshot_raw.proto snapshot/snapshot_rpc/snapshot.swagger
	go generate -x ./snapshot/snapshot_rpc

grpc-python: grpc-generators
	cpp -DPYTHON -P -traditional-cpp -nostdinc snapshot/snapshot_rpc/snapshot_raw.proto snapshot/snapshot_rpc/snapshot_python.proto
	cd python_client; python -m grpc_tools.protoc -I.:../vendor \
	    --python_out=. --grpc_python_out=. \
	    ./yc_snapshot_client/snapshot_rpc/snapshot_python.proto

swagger: grpc-go

rpc: grpc-go grpc-python swagger


# For CI
create-tmpdir:
	@if [ ! -d "$(TEST_TMPDIR)" ]; then \
		mkdir -p $(TEST_TMPDIR); \
		sed -i "s,Tmpdir =.*,Tmpdir = \"$(TEST_TMPDIR)\",g" $(TEST_CONFIG_PATH); \
		sed -i "s,^Root =.*,Root = \"yctest2/ycloud/rekby/test-yc-teamcity/${BUILD_NUMBER}\",g" $(TEST_CONFIG_PATH); \
	fi

populate-database:
	go run ./tools/populate-database/main.go --config=$(TEST_CONFIG_PATH)

testcover: create-tmpdir populate-database
	@echo "##teamcity[blockOpened name='go test']"
	go install ./vendor/github.com/2tvenom/go-test-teamcity
	set -o pipefail; TEST_CONFIG_PATH=$(TEST_CONFIG_PATH) ./run_tests.sh $(PKGS) | $(GOTESTTC)
	@echo "##teamcity[blockClosed name='go test']"
	@echo "##teamcity[blockOpened name='go test coverage']"
	go tool cover -func=coverage.txt 2>/dev/null
	go tool cover -func=coverage.txt -o funccoverage.txt 2>/dev/null
	go tool cover -html=coverage.txt -o coverage.html 2>/dev/null
	go tool cover -html=coverage.txt -o index.html 2>/dev/null
	@echo "##teamcity[blockClosed name='go test coverage']"

python-tests: build create-tmpdir populate-database
	@echo "##teamcity[blockOpened name='python tests']"
	@echo "Starting service"
	./yc-snapshot --config=$(TEST_CONFIG_PATH) >/dev/null & echo "$$!" > $(PIDFILE)
	# Wait for snapshot service start
	sleep 5
	@echo "Running tests"
	cd python_client; python setup.py test; echo $$? > $(RESULTFILE)
	-kill $$(cat $(PIDFILE))
	rm -f $(PIDFILE)
	@echo "##teamcity[blockClosed name='python tests']"
	@[ $$(cat $(RESULTFILE)) -eq 0 ];

autotests: fmt vet testcover python-tests
