CHART_PATH ?= cr.yandex/crp136vhf7jvu9cnpvga/charts/namespaces

export HELM_EXPERIMENTAL_OCI := 1

ifneq ($(CHART_VERSION),)
HELM_PACKAGE_ARGS += --version $(CHART_VERSION)
endif

ifneq ($(APP_VERSION),)
HELM_PACKAGE_ARGS += --app-version $(APP_VERSION)
endif

helm-release:
	HELM_ARCHIVE=`helm package $(HELM_PACKAGE_ARGS) . -d out/  | grep -o 'out/.*.tgz'`; \
	HELM_IMAGE=`helm chart save $$HELM_ARCHIVE $(CHART_PATH) | grep -o '$(CHART_PATH).*'`; \
	helm chart push $$HELM_IMAGE
