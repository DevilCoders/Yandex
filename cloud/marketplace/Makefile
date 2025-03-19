# api and queue targets are only for testing custom code on preprod
.PHONY: queue api
SHELL := bash
BRANCH := $(shell arc st | grep -m1 branch | cut -d' ' -f3 | cut -d'/' -f2)

DOCKER_IMAGE = cr.yandex/crpo2khtm9ocefi307m8/marketplace-v1
DATE = $(shell date "+%d.%m.%Y.%s")
VERSION = ${BRANCH}-${DATE}

ydb:
	bash -e misc/utils/misc/ydb.sh;

build:
	ya make .

queue: build
	cp queue/bin/yc_marketplace_queue yc_marketplace_queue
	cp cli/bin/yc_marketplace_cli yc_marketplace_cli
	docker build -t $(DOCKER_IMAGE)-worker:$(VERSION) -f packages/Dockerfile-queue .
	docker push $(DOCKER_IMAGE)-worker:$(VERSION)
	rm yc_marketplace_cli yc_marketplace_queue

api: build
	cp api/wsgi/yc_marketplace yc_marketplace
	cp cli/bin/yc_marketplace_cli yc_marketplace_cli
	docker build -t $(DOCKER_IMAGE):$(VERSION) -f packages/Dockerfile-api .
	docker push $(DOCKER_IMAGE):$(VERSION)
	rm yc_marketplace_cli yc_marketplace
