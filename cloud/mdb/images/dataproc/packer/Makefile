BASE_IMAGE ?= packer.dataproc-base-2004.json
IMAGE ?= packer.dataproc-image-2004.json
ENV ?= preprod
ON_ERROR ?= cleanup
COMMIT ?= $(shell arc log --oneline | head -n 1 | cut -f1 -d' ')
YA_BRANCH ?= $(shell arc branch 2> /dev/null | sed -e 's/\//-/' | sed -e '/^[^*]/d' -e 's/\* //' | tr '[:upper:]' '[:lower:]' | cut -c1-63)
MAKEFILE_DIRECTORY = $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
PYTHON_COMMON_PATH = ${MAKEFILE_DIRECTORY}/../../../../bitbucket/python-common
YC_SERVICE_ACCOUNT_KEY_FILE ?= key.json

ifdef PROFILE
ENV = $(PROFILE)
TOKEN = $(shell yc --profile $(PROFILE) config get token)
endif

.PHONY=clean
clean:
	rm -f packer-manifest.json

.PHONY=validate-base
validate-base:
	@echo "Start validation ${BASE_IMAGE} with variables-${ENV}.json"
	packer validate -var-file=variables-${ENV}.json ${BASE_IMAGE}

.PHONY=validate-image
validate-image:
	@echo "Start validation ${IMAGE} with variables-${ENV}.json"
	packer validate -var-file=variables-${ENV}.json ${IMAGE}

.PHONY=validate
validate: validate-base validate-image

.PHONY=fix-base
fix-base:
	packer fix ${BASE_IMAGE}

.PHONY=fix-image
fix-image:
	packer fix ${IMAGE}

.PHONY=fix
fix: fix-base fix-image

key.json:
	ya vault get version sec-01f21mtzjv44s5sv54whxc3gfw --only-value key.json > key.json

.PHONY=build
dataproc-base: validate-base
	@echo "Start building ${BASE_IMAGE} with variables-${ENV}.json"
	@YC_SERVICE_ACCOUNT_KEY_FILE=key.json COMMIT=${COMMIT} YA_BRANCH=${YA_BRANCH} packer build -timestamp-ui -var-file=variables-${ENV}.json ${BASE_IMAGE}

.PHONY=build
dataproc-image: validate-image
	@echo "Start building ${IMAGE} with variables-${ENV}.json"
	@YC_SERVICE_ACCOUNT_KEY_FILE=key.json COMMIT=${COMMIT} YA_BRANCH=${YA_BRANCH} packer build -timestamp-ui -on-error=${ON_ERROR} -except export_to_s3 -var-file=variables-${ENV}.json ${IMAGE}
