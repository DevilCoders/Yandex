export MDB_PROJECT_NAME=mdb-secrets
export MDB_PROJECT_CMD_PATH=cmd/${MDB_PROJECT_NAME}/${MDB_PROJECT_NAME}

include ../Makefile.defs

run:
	${MDB_PROJECT_CMD_PATH} --scheme=http --port=8080 --config-path=./

.PHONY: run
