projects = \
	deploy/api \
	mdb-dns \
	mdb-health \
	cms/api \
	internal/crt/cloudcrt \
	internal/juggler/http \
	mdb-secrets

OS = $(shell echo $(shell uname -s) | tr A-Z a-z)
export MDB_PROJECTS = $(projects)

$(projects):
	$(MAKE) -C $@ $(MAKECMDGOALS)

generate-swagger: $(projects)

clean-swagger: $(projects)

generate:
	ya tool go generate ./...

all:
	$(MAKE) generate-swagger
	$(MAKE) generate
	$(MAKE) generate-proto

generate-proto:
	ya make --add-result=pb.go

.PHONY: generate generate-swagger generate-proto clean-swagger $(projects)
