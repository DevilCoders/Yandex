## Overview

Example utility to copy service dashboards from user dashboards and back. For more information see wiki: https://wiki.yandex-team.ru/solomon/service-providers/.

## Configure

### Preprod

Docs: https://cloud.yandex.ru/docs/iam/operations/iam-token/create

```
$ yc config profile get preprod-fed-user
federation-id: yc.yandex-team.federation
endpoint: api.cloud-preprod.yandex.net:443
federation-endpoint: console-preprod.cloud.yandex.ru
compute-default-zone: ru-central1-b

$ export MONITORING_IAM_TOKEN=$(yc --profile=preprod-fed-user iam create-token)
```

### Prod

Docs: https://cloud.yandex.ru/docs/iam/operations/iam-token/create

`export MONITORING_IAM_TOKEN=$(yc --endpoint api.cloud.yandex.net:443 iam create-token)`

### Porto
1. Update internal certificates. Docs: https://a.yandex-team.ru/arc/trunk/arcadia/certs/update-certs.py

`$ ../../../certs/update-certs.py`

2. Define OAuth token. Docs: https://solomon.yandex-team.ru/docs/api-ref/authentication#oauth
`export SOLOMON_OAUTH_TOKEN="<token>"`

## Build
`$ ya make`

## Run

### 1. Create an empty service dashboard

`$ yc/v3api_dashboards -e preprod --service <your service> create-empty`

Parameter `--service` should contain your service provider id, which is the same with the `service` parameter you use to report metrics to Monitoring: managed-mysql, managed-kafka, etc

**NB!** Each run creates new dashboard, do not create multiple dashboards if you do not need them!

___
### 2. Copy definition from regular dashboard

`$ yc/v3api_dashboards -e preprod copy --service-dashboard-id mon1rohrb4r6s8cv83im --dashboard-id biiu3soj0k8j920nru2v`

For prod use `--environment prod` parameter. Be careful when working with prod! And do not forget to review your changes with Monitoring team!

___
### 3. Create service dashboard from config

`$ yc/v3api_dashboards -e preprod --service <your service> create --config <config path>`

___
### 4. Update dashboard from config
Update service dashboard:

`$ yc/v3api_dashboards -e preprod --service <your service> update --config <config path>`

Update regular dashboard:

`$ yc/v3api_dashboards -e porto update --config dashboards/porto/postgresql-cluster.json`

___
### 5. Sync porto dashboard from prod service dashboard

`$ yc/v3api_dashboards -e prod -e1 porto --service-dashboard-id 9m6nosvxnop67 --dashboard-id mon9obabhq5tvnrpcj0g --debug sync`

---
### 6. Save dashboard definition

`$ yc/v3api_dashboards -e preprod -s biioi6o8dhtvd12jij39 get > dashboards/compute_preprod/clickhouse-cluster.json`
