BASE_IMAGE ?= packer.dataproc-base-1604.json
IMAGE ?= packer.dataproc-image-1604.json
ENV ?= preprod
ON_ERROR ?= cleanup
COMMIT ?= $(shell arc log --oneline | head -n 1 | cut -f1 -d' ')
YA_BRANCH ?= $(shell arc branch 2> /dev/null | sed -e '/^[^*]/d' -e 's/\* //' | tr '[:upper:]' '[:lower:]')
MAKEFILE_DIRECTORY = $(dir $(realpath $(firstword $(MAKEFILE_LIST))))
PYTHON_COMMON_PATH = ${MAKEFILE_DIRECTORY}/../../../../bitbucket/python-common
PROFILE ?= "default"
TOKEN = $(shell yc --profile $(PROFILE) config get token)

venv:
	python3 -m venv venv
	./venv/bin/pip --no-cache-dir install -i https://pypi.yandex-team.ru/simple yandex-passport-vault-client==1.4.0 retry
	./venv/bin/pip --no-cache-dir install --extra-index-url https://pypi.python.org/simple -i https://pypi.yandex-team.ru/simple -r ${PYTHON_COMMON_PATH}/requirements.txt
	./venv/bin/pip --no-cache-dir install ${PYTHON_COMMON_PATH}

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

.PHONY=build
dataproc-base: validate-base
	@echo "Start building ${BASE_IMAGE} with variables-${ENV}.json"
	@YC_SERVICE_ACCOUNT_KEY_FILE=key.json COMMIT=${COMMIT} YA_BRANCH=${YA_BRANCH} packer build -timestamp-ui -var-file=variables-${ENV}.json ${BASE_IMAGE}

.PHONY=build
dataproc-image: validate-image venv allCAs.pem
	@echo "Start building ${IMAGE} with variables-${ENV}.json"
	@YC_SERVICE_ACCOUNT_KEY_FILE=key.json COMMIT=${COMMIT} YA_BRANCH=${YA_BRANCH} packer build -timestamp-ui -on-error=${ON_ERROR} -except export_to_s3 -var-file=variables-${ENV}.json ${IMAGE}
	@echo "Rewriting image attributes and creating NBS disk pools for ${IMAGE} with variables-${ENV}.json"
	REQUESTS_CA_BUNDLE=allCAs.pem venv/bin/python images.py create --variables variables-${ENV}.json
