#/usr/bin/env make

all: build up  #migrate ydb start is wery long story so run manualy

build:
	ya package --docker pkg.json

up:
	docker-compose up -d lbk

down:
	docker-compose down
