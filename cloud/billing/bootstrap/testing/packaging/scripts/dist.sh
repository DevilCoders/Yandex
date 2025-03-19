PREPROD_ENDPOINT="https://storage.cloud-preprod.yandex.net"
PROD_ENDPOINT="https://storage.yandexcloud.net"

cd `dirname $0`

set -e

. /var/run/preprod_aws.key
./aws --endpoint-url $PREPROD_ENDPOINT --no-verify-ssl s3 cp --recursive preprod/ s3://yc-billing-configs/selfsku-test/ | tee .log
grep -q 'upload failed' .log && exit 99 || true

. /var/run/prod_aws.key
./aws --endpoint-url $PROD_ENDPOINT --no-verify-ssl s3 cp --recursive prod/ s3://yc-billing-configs/selfsku-test/ | tee .log
grep -q 'upload failed' .log && exit 99 || true
