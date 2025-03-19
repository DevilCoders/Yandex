
REPO=a.yandex-team.ru/cloud/compute/go-common
PKGS := $(shell go list ./... | grep -v ^${REPO}/vendor/)

.PHONY: fmt vet test
.DEFAULT: all

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
	@go test -test.short $(PKGS)
