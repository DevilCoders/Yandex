#!/bin/bash

set -e

secret_uuid='sec-01fn90nj47vrpzj8hxg3md774k'
version=$(yav get secret ${secret_uuid} -j|jq -r 'values.secret_versions[0].version')
data=$(yav get version ${version} -j)

username=$(echo $data|jq -r values.value.user)
password=$(echo $data|jq -r values.value.password)

for repo in cloudbeaver cloudbeaver-ee cloudbeaver-yandex dbeaver dbeaver-ee dbeaver-resources-clients-native dbeaver-resources-drivers-jdbc
do
    if [ -d "${repo}" ];then
        cd ${repo}
        git pull
        cd ..
    else
        git clone https://${username}:${password}@github.com/dbeaver/${repo}
    fi
done

