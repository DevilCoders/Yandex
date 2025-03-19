set -e

export TOKEN=`curl -s -H "Metadata-Flavor: Google" http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token|jq -r .access_token`

echo downloading common
curl -s -H "X-YaCloud-SubjectToken: ${TOKEN}" https://storage.cloud.yandex.net/yc-yandex-billing-configs/selfsku-ya/common -o common && chmod a+x common
