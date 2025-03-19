#!/bin/sh

YAV_TOKEN=`cat ~/.ssh/yav_token`
YC_TOKEN=`yc --profile trail-preprod iam create-token`

# Run terraform
terraform $* -var yandex_token=${YAV_TOKEN} -var yc_token=${YC_TOKEN}
