export MDB_SWAGGER_NO_SERVER=1
export MDB_SWAGGER_CMD_NAME=cloudcrt

include ../../../Makefile.defs

update-swagger:
	wget --no-check-certificate https://crt.cloud.yandex.net/swagger/doc.json -O api/swagger.yaml
	swagger flatten --with-flatten full api/swagger.yaml -o api/swagger.yaml
