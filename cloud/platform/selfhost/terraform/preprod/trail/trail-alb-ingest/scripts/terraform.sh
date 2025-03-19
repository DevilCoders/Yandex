#!/bin/sh

YAV_TOKEN=`cat ~/.ssh/yav_token`

# Run terraform
terraform $* -var yandex_token=${YAV_TOKEN}

