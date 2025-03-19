cd `dirname $0`

set -e

. /var/run/ya_pre_aws.key
./aws --endpoint-url "https://storage.yandexcloud.net" --no-verify-ssl s3 cp --recursive preprod/ s3://yc-yandex-billing-preprod-configs/selfsku-ya/ | tee .log
grep -q 'upload failed' .log && exit 99 || true

. /var/run/ya_aws.key
./aws --endpoint-url "https://storage.yandexcloud.net" --no-verify-ssl s3 cp --recursive prod/ s3://yc-yandex-billing-configs/selfsku-ya/ | tee .log
grep -q 'upload failed' .log && exit 99 || true
