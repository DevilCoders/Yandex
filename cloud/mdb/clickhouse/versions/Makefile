.PHONY: all
all: build generate

.PHONY: build
build:
	ya make .

.PHONY: generate
generate: build
	env LC_ALL=C.UTF-8 LANG=C.UTF-8 config_generator/config-generator

.PHONY: pr
pr: generate
	arc add --all && arc commit -m "Update of ClickHouse versions [clickhouse]" && arc pr create --push --no-edit

.PHONY: test
test: build
	env LC_ALL=C.UTF-8 LANG=C.UTF-8 config_generator/config-generator --check || \
	    (echo 'ERROR: Unexpected changes in generated files. Please make sure that "make generate" has been run and generated changes have been commited.' >&2; exit 1)
