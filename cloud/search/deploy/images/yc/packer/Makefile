BASE_IMAGE ?= search-base-2if-1804.json
ENV ?= preprod
ON_ERROR ?= clean

ifdef PROFILE
ENV = $(PROFILE)
endif

.PHONY=make-env
make-env:


allCAs.pem:
	wget https://crls.yandex.net/allCAs.pem

.PHONY=clean
clean:
	rm -rf python-common
	rm -rf venv
	rm -f packer-manifest.json
	rm -f allCAs.pem

.PHONY=validate-base
validate-base:
	@echo "Start validation ${BASE_IMAGE} with variables-${ENV}.json"
	packer validate -var-file=variables-${ENV}.json ${BASE_IMAGE}

.PHONY=validate
validate: validate-base

.PHONY=gen-secrets
gen-serets: make-env
	@echo "Start generating secrets for ${ENV}."
	@bash ./generate-secrets.sh ${ENV}

.PHONY=build
build-base: gen-serets validate-base allCAs.pem
	@echo "Start building ${BASE_IMAGE} with variables-${ENV}.json"
	packer build -timestamp-ui -var-file=variables-${ENV}.json ${BASE_IMAGE}
