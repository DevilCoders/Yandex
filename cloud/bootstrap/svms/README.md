# Templates for bootstrap svms

Based on https://wiki.yandex-team.ru/cloud/devel/selfhost/bootstrap-host/

## Requirements
 * ycp with prod profile for yandexcloud cloud and yndx-ycprod-bs token

1. Prepare yaml request
    ```
    selfdns_token=AQAD-xxxx envsubst < ${template} >./tmp/$(basename ${template})
    ```
2. Create VM
    ```
    ycp compute instance create -r ./tmp/${template}
    ```
