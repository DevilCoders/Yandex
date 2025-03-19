# This makefile used to generate encrypted keys for SA with editor role in load test Folder
# SA used by hoper for moving images from other environments.
# https://wiki.yandex-team.ru/cloud/devel/assembly-workshop/welcome/#packer-with-hopper

SERVICE_ACCOUNT:=yc.vpc.load.test.hopper.sa
TMP_DIR:=/var/tmp/skm-hopper-keys

skm:
	mkdir -p ${TMP_DIR}
	# yc --profile preprod iam key create --service-account-id ${SERVICE_ACCOUNT} -o "${TMP_DIR}/preprod.sa.json"
	yc --profile prod iam key create --service-account-id ${SERVICE_ACCOUNT} -o "${TMP_DIR}/prod.sa.json"
	YC_TOKEN=`yc --profile prod iam create-token` skm encrypt-bundle --config skm.yaml --bundle skm-encrypted-keys.yaml