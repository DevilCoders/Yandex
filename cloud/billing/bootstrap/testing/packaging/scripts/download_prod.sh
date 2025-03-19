set -e

export TOKEN=`curl -s -H "Metadata-Flavor: Google" http://169.254.169.254/computeMetadata/v1/instance/service-accounts/default/token|jq -r .access_token`

echo downloading test_bundle
curl -s -H "X-YaCloud-SubjectToken: ${TOKEN}" https://storage.cloud.yandex.net/yc-billing-configs/selfsku-test/test_bundle -o test_bundle && chmod a+x test_bundle
