.PHONY: build build_service_images build_dev deploy_dev clean fill_values clean_bin gen_env

ARCADIA_ROOT=${PWD}/../../..
ARC_REVISION_HASH=$(shell arc info --json | jq '.hash')
ARC_BRANCH=$(shell arc info --json | jq '.branch')

HELMFILE_DIR_PATH ?= staging/helm/
IMAGE_TAG?=${ARC_REVISION_HASH}
STAND?=${IMAGE_TAG}
IMAGE_REGISTRY?=cr.cloud-preprod.yandex.net/crtv8pkp25ef8uvt2std

define build_image
	${ARCADIA_ROOT}/ya package $(1) \
	      --docker \
	      --docker-registry=${IMAGE_REGISTRY} \
	      --custom-version=${IMAGE_TAG} \
	      --package-output=staging/packages \
	      --docker-push
endef

build: build_service_images infratests staging/bin/yc

infratests:
	ya make ${YA_MAKE_OPTS}

clean_bin:
	@rm infratests

build_service_images: cmsdb-migrations dataproc-manager dbaas-worker deploy-api deploy-cleaner deploy-salt deploydb-migrations dist-sync \
dns e2e-tests health internal-api katandb-migrations logs-api maintenance metadb-migrations mlock mlockdb-migrations \
pillar-config pillar-secrets py-api release-promote secrets-api secretsdb-migrations tricorder ui vpc-api vpc-worker \
vpcdb-migrations

cmsdb-migrations:
	$(call build_image, '../datacloud/docker/cmsdb-migrations/pkg.json')
dataproc-manager:
	$(call build_image, '../bootstrap/docker/dataproc-manager/pkg.json')
dbaas-worker:
	$(call build_image, '../datacloud/docker/dbaas-worker/pkg.json')
deploy-api:
	$(call build_image, '../datacloud/docker/deploy-api/pkg.json')
deploy-cleaner:
	$(call build_image, '../datacloud/docker/deploy-cleaner/pkg.json')
deploy-salt:
	$(call build_image, '../datacloud/docker/deploy-salt/pkg.json')
deploydb-migrations:
	$(call build_image, '../datacloud/docker/deploydb-migrations/pkg.json')
dist-sync:
	$(call build_image, '../datacloud/docker/dist-sync/pkg.json')
dns:
	$(call build_image, '../datacloud/docker/dns/pkg.json')
e2e-tests:
	$(call build_image, '../datacloud/docker/e2e-tests/pkg.json')
health:
	$(call build_image, '../datacloud/docker/health/pkg.json')
internal-api:
	$(call build_image, '../datacloud/docker/internal-api/pkg.json')
katandb-migrations:
	$(call build_image, '../datacloud/docker/katandb-migrations/pkg.json')
logs-api:
	$(call build_image, '../datacloud/docker/logs-api/pkg.json')
maintenance:
	$(call build_image, '../datacloud/docker/maintenance/pkg.json')
metadb-migrations:
	$(call build_image, '../datacloud/docker/metadb-migrations/pkg.json')
mlock:
	$(call build_image, '../datacloud/docker/mlock/pkg.json')
mlockdb-migrations:
	$(call build_image, '../datacloud/docker/mlockdb-migrations/pkg.json')
pillar-config:
	$(call build_image, '../datacloud/docker/pillar-config/pkg.json')
pillar-secrets:
	$(call build_image, '../datacloud/docker/pillar-secrets/pkg.json')
py-api:
	$(call build_image, '../datacloud/docker/py-api/pkg.json')
release-promote:
	$(call build_image, '../datacloud/docker/release-promote/pkg.json')
secrets-api:
	$(call build_image, '../datacloud/docker/secrets-api/pkg.json')
secretsdb-migrations:
	$(call build_image, '../datacloud/docker/secretsdb-migrations/pkg.json')
tricorder:
	$(call build_image, '../datacloud/docker/tricorder/pkg.json')
ui:
	$(call build_image, '../datacloud/docker/ui/pkg.json')
vpc-api:
	$(call build_image, '../datacloud/docker/vpc-api/pkg.json')
vpc-worker:
	$(call build_image, '../datacloud/docker/vpc-worker/pkg.json')
vpcdb-migrations:
	$(call build_image, '../datacloud/docker/vpcdb-migrations/pkg.json')

staging/bin/yc:
	mkdir -p staging
	wget https://s3.mds.yandex.net/mcdev/internal-cli/install.sh -q -O staging/yc_install.sh
	chmod +x staging/yc_install.sh
	bash ./staging/yc_install.sh -n -i ./staging/

build_dev: build

deploy_dev:
	IMAGE_TAG=${IMAGE_TAG} IMAGE_REGISTRY=${IMAGE_REGISTRY} NAMESPACE=${STAND} \
		helmfile --file="${HELMFILE_DIR_PATH}/helmfile.yaml" apply

clean:
	rm -rf "staging"
	ACTION=delete_stand ./infratests provision
	kubectl --context yc-compute-preprod-infratest delete namespace ${STAND}

fill_values: infratests
	ACTION=fill_values ./infratests provision

gen_env:
	@echo "Clone https://review.db.yandex-team.ru/admin/repos/mdb/controlplane"
	@read -p "Enter path to mdb/controlplane git repo on your machine: " cp_repo; \
	echo "export HELMFILE_DIR_PATH=$${cp_repo}/helm/obsolete_stands/yc-compute-preprod-dev/infra-test/" > .envrc; \
	echo "export STAND=${USER}" >> .envrc
