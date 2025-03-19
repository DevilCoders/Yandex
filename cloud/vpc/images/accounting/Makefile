# This makefile used to generate encrypted keys for SA with editor role in accounting Folder
# SA used by hoper for moving images from other environments.
# https://wiki.yandex-team.ru/cloud/devel/assembly-workshop/welcome/#packer-with-hopper

SERVICE_ACCOUNT:=yc.vpc.accounting.hopper.sa
TMP_DIR:=/var/tmp/skm-hopper-keys
JQ_FILTER:='.key + {private_key} | del(.fingerprint)'
CWD=$(shell pwd)
IMAGE_UPDATER=${CWD}/../utils/update_image_id.py

update_image_id:
	${IMAGE_UPDATER} --hopper-json ${CWD}/images.json  --target-var accounting_image --target ${CWD}/../../terraform-configs/preprod/accounting/main.tf --env PREPROD
	${IMAGE_UPDATER} --hopper-json ${CWD}/images.json  --target-var accounting_image --target ${CWD}/../../terraform-configs/prod/accounting/main.tf --env PROD
	${IMAGE_UPDATER} --hopper-json ${CWD}/images.json  --target-var accounting_image --target ${CWD}/../../terraform-configs/israel/accounting/main.tf --env IL

skm:
	mkdir -p ${TMP_DIR}
	ycp --profile preprod iam key create --service-account-id ${SERVICE_ACCOUNT} --format json | jq ${JQ_FILTER} > "${TMP_DIR}/preprod.sa.json"
	ycp --profile prod iam key create --service-account-id ${SERVICE_ACCOUNT} --format json | jq ${JQ_FILTER} > "${TMP_DIR}/prod.sa.json"
	ycp --profile israel iam key create --service-account-id ${SERVICE_ACCOUNT} --format json | jq ${JQ_FILTER} > "${TMP_DIR}/israel.sa.json"
	YC_TOKEN=`yc --profile prod iam create-token` skm encrypt-bundle --config skm.yaml --bundle skm-encrypted-keys.yaml