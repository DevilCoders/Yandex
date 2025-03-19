CHART_PATH ?= oci://cr.yandex/crp136vhf7jvu9cnpvga/charts

export HELM_EXPERIMENTAL_OCI := 1

APP_VERSION ?= $(shell arc log -n 1 --oneline --json | jq -r '.[0].commit')
CHART_VERSION ?= $(addsuffix -$(APP_VERSION), $(shell yq e '.version' Chart.yaml))

ifneq ($(CHART_VERSION),)
HELM_PACKAGE_ARGS += --version $(CHART_VERSION)
endif


ifneq ($(APP_VERSION),)
HELM_PACKAGE_ARGS += --app-version $(APP_VERSION)
endif

helm-release: clean
	HELM_ARCHIVE=`helm package $(HELM_PACKAGE_ARGS) . -d out/  | grep -o 'out/.*.tgz'`; \
	helm push $$HELM_ARCHIVE $(CHART_PATH)

clean:
	rm -rf ./out/*
