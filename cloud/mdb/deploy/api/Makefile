export MDB_PROJECT_NAME=mdb-deploy-api
export MDB_SWAGGER_CMD_NAME=mdb-deployapi
export MDB_PROJECT_CMD_PATH=cmd/${MDB_PROJECT_NAME}/${MDB_PROJECT_NAME}

include ../../Makefile.defs

run:
	${MDB_PROJECT_CMD_PATH} --scheme=http --config-path=~

.PHONY: run
