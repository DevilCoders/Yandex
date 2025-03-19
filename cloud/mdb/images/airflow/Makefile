TOKEN ?= $(shell ya vault get version ver-01e0qfg7v7m8vfgr04va21e4qc -o token)

ifdef PROFILE
ENV = $(PROFILE)
TOKEN = $(shell yc --profile $(PROFILE) config get token)
endif

.PHONY=build

build-image-2:
	@echo "Start building airflow-ubuntu-2004.packer.json with airflow2-variables.json"
	@TOKEN=${TOKEN} packer build -timestamp-ui -var-file=airflow2-variables.json airflow-ubuntu-2004.packer.json
