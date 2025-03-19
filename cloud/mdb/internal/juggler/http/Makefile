export MDB_SWAGGER_NO_SERVER=1
export MDB_SWAGGER_CMD_NAME=juggler

include ../../../Makefile.defs

update-swagger:
	wget --no-check-certificate https://juggler.yandex-team.ru/api2/swagger.yaml -O api/swagger.yaml
	swagger flatten --with-flatten full api/swagger.yaml > api/swagger.yaml
