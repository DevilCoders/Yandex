## Кондукторные группы стендов
* Internal-Dev [C@cloud_prod_iam-internal-dev](https://c.yandex-team.ru/groups/cloud_prod_iam-internal-dev)
* Testing [C@cloud_testing_iam-as](https://c.yandex-team.ru/groups/cloud_testing_iam-as)
* Internal-Prestable [C@cloud_prod_iam-internal-prestable](https://c.yandex-team.ru/groups/cloud_prod_iam-internal-prestable)
* Preprod [C@cloud_preprod_iam-as](https://c.yandex-team.ru/groups/cloud_preprod_iam-as)
* DataCloud-Preprod: cluster eks_iam-preprod
* Internal-Prod [C@cloud_prod_iam-ya](https://c.yandex-team.ru/groups/cloud_prod_iam-ya)
* Prod [C@cloud_prod_iam-as](https://c.yandex-team.ru/groups/cloud_prod_iam-as)
* Israel `x@iam-as-*.svc.yandexcloud.co.il`
* DataCloud-Prod: cluster eks_iam-prod


## Релиз
1. Порядок выкатки: Testing -> Preprod -> Prod
    * Testing: Internal-dev -> Testing
    * Preprod: Internal-Prestable -> Preprod -> DataCloud-Preprod
    * Prod: Internal-Prod -> PROD & Israel -> DataCloud-Prod
3. После каждого стенда надо посмотреть на мониторинги и логи, убедиться, что все ок. После этого тикет переводится в состояние Testing in Dev/Testing in PreProd/Released в зависимости от стенда.
4. Прод сначала выкатывается на одну машину, потом в одну AZ, потом все остальные.
5. Если на проде что-то пошло не так в процессе выкатки на машину или AZ, то сначала нужно погасить на них сервисы, потом разбираться.

[Инструкция по деплою](https://wiki.yandex-team.ru/cloud/devel/deploy/)

1. Создать [заявку на релиз](https://forms.yandex-team.ru/surveys/11002)
2. Версию релиза берём в сборке [salt-formula](https://teamcity.yandex-team.ru/buildConfiguration/Cloud_Packages_Platform_SaltFormula?branch=%3Cdefault%3E&mode=builds#all-projects) и указываем в релизном тикете.
3. Прописываем версию в cluster-configs (дока [тут](https://wiki.yandex-team.ru/cloud/devel/selfhost/salt-roles-deploy/#ustanovkarelizakotoryjjkatim)).
   * [testing/base-role-releases/iam-access-service.yaml](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/testing/base-role-releases/iam-access-service.yaml)
   * [prod/base-role-releases/iam-internal-dev.yaml](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/prod/base-role-releases/iam-internal-dev.yaml)
   * Позже, вместо move-release - [iam-internal-prestable.yaml](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/prod/base-role-releases/iam-internal-prestable.yaml) и [iam-ya.yaml](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/prod/base-role-releases/iam-ya.yaml)
      
4. Зайти на `pssh bootstrap.cloud.yandex.net`



# testing phase
## Internal-dev
Downtime [устанавливаем](https://juggler.yandex-team.ru/downtimes/) для `host=yc_iam_svm_internal-dev`

{% cut "Шаблон процесса деплоя" %}

    ===[Internal-dev] Access service (<salt-formula-ver>:<yc-access-service-ver>)
    **common** <{TODO
    ```(bash)
    yc-bootstrap --apply --template workflows/prod/common/update/iam-internal-dev.yaml --filter %iam-internal-dev --ticket-id ${TICKET_ID}
    ```
    <[
    ]>
    }>

    **iam-as** <{TODO
    ```(bash)
    yc-bootstrap --apply --template workflows/prod/iam-ya-access-service/update/iam-internal-dev.yaml --filter %iam-internal-dev --ticket-id ${TICKET_ID}
    ```
    <[
    ]>
    }>

{% endcut %}

1. move release не делается, версия прописана в [prod/base-role-releases/iam-internal-dev.yaml](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/prod/base-role-releases/iam-internal-dev.yaml) на этапе подготовке к релизу.
1. common (по большим праздникам)
   ```bash
   yc-bootstrap --apply --template workflows/prod/common/update/iam-internal-dev.yaml --filter %iam-internal-dev --ticket-id ${TICKET_ID}
   ```
1. service
   ```bash
    # на стенд Internal-dev
    yc-bootstrap --apply --template workflows/prod/iam-ya-access-service/update/iam-internal-dev.yaml --filter %iam-internal-dev --ticket-id ${TICKET_ID}
    ```



## Testing
Downtime [устанавливаем](https://juggler.yandex-team.ru/downtimes/) для `host=yc_iam_as_svm_testing`

{% cut "Шаблон процесса деплоя" %}

    ===[Testing] Access service (<salt-formula-ver>:<yc-access-service-ver>)

    **common** <{TODO
    ```(bash)
    yc-bootstrap --apply --template workflows/testing/common/update/iam-access-service.yaml --filter %iam-access-service --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-myt1** <{TODO
    ```(bash)
    yc-bootstrap --apply --template workflows/testing/iam-access-service/update/allaz.yaml --filter host=iam-as-myt1.svc.cloud-testing.yandex.net --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as** <{TODO
    ```(bash)
    yc-bootstrap --apply --template workflows/testing/iam-access-service/update/allaz.yaml --filter "%iam-access-service -iam-as-myt1.svc.cloud-testing.yandex.net" --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

{% endcut %}

Следим за [сервисами](https://grafana.yandex-team.ru/d/iam-duty-deploy/iam-duty-deploy?orgId=1&from=now-3h&to=now&refresh=1m&var-appFilter=*_server&var-ds=Solomon%20Cloud%20Preprod&var-cluster=testing&var-host=All) и за [выкатываемыми нодами](https://grafana.yandex-team.ru/d/iam-duty_dd-grpc-by-host/iam-grpc-by-hosts?orgId=1&from=now-3h&to=now&var-service=access-service&var-ds=Solomon%20Cloud%20Preprod&var-cluster=testing&var-host=All&var-app=access-service_server&var-method=All&refresh=1m).

* Update common:
    1. На одной машине:
        ```(bash)
        yc-bootstrap --apply --template workflows/testing/common/update/iam-access-service.yaml --filter host=iam-as-myt1.svc.cloud-testing.yandex.net --ticket-id ${TICKET_ID}
        ```
    1. На всех:
        ```(bash)
        yc-bootstrap --apply --template workflows/testing/common/update/iam-access-service.yaml --filter "%iam-access-service -iam-as-myt1.svc.cloud-testing.yandex.net" --ticket-id ${TICKET_ID}
        ```

* Update access-service:
    1. На одной ноде:
        ```(bash)
        yc-bootstrap --apply --template workflows/testing/iam-access-service/update/allaz.yaml --filter host=iam-as-myt1.svc.cloud-testing.yandex.net --ticket-id ${TICKET_ID}
        ```
    1. На всех:
        ```(bash)
        yc-bootstrap --apply --template workflows/testing/iam-access-service/update/allaz.yaml --filter "%iam-access-service -iam-as-myt1.svc.cloud-testing.yandex.net" --ticket-id ${TICKET_ID}
        ```



# pre-prod phase
## Internal-prestable
Downtime [устанавливаем](https://juggler.yandex-team.ru/downtimes/) для `host=yc_iam_svm_internal-prestable`

{% cut "Шаблон процесса деплоя" %}

    ===[Internal-prestable] Access Service (<salt-formula-ver>:<yc-access-service-ver>)

    **common** <{TODO
    ```(bash)
    yc-bootstrap --apply --template workflows/prod/common/update/iam-internal-prestable.yaml --filter %iam-internal-prestable --ticket-id ${TICKET_ID}
    ```
    <[
    ]>
    }>

    **iam-as** <{TODO
    ```(bash)
    yc-bootstrap --apply --template workflows/prod/iam-ya-access-service/update/iam-internal-prestable.yaml --filter %iam-internal-prestable --ticket-id ${TICKET_ID}
    ```
    <[
    ]>
    }>

{% endcut %}

1. move release
   * Меняем версию пакетов в [prod/base-role-releases/iam-internal-prestable.yaml](https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/prod/base-role-releases/iam-internal-prestable.yaml)
1. common (по большим праздникам)
   ```bash
   yc-bootstrap --apply --template workflows/prod/common/update/iam-internal-prestable.yaml --filter %iam-internal-prestable --ticket-id ${TICKET_ID}
   ```
1. service
   ```bash
    # на стенд Internal-prestable
    yc-bootstrap --apply --template workflows/prod/iam-ya-access-service/update/iam-internal-prestable.yaml --filter %iam-internal-prestable --ticket-id ${TICKET_ID}
    ```



## Preprod
Downtime [устанавливаем](https://juggler.yandex-team.ru/downtimes/) для `host=yc_iam_as_svm_preprod`

{% cut "Шаблон процесса деплоя" %}

    ===[Preprod] Access Service (<salt-formula-ver>:<yc-access-service-ver>)

    **common iam-as-myt1** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/pre-prod/common/update/iam-access-service.yaml --filter host=iam-as-myt1.svc.cloud-preprod.yandex.net --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **common except iam-as-myt1** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/pre-prod/common/update/iam-access-service.yaml --filter "%iam-access-service -iam-as-myt1.svc.cloud-preprod.yandex.net" --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **move** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/pre-prod/iam-access-service/move-base-release/from-testing.yaml --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-myt1** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/pre-prod/iam-access-service/update/allaz.yaml --filter host=iam-as-myt1.svc.cloud-preprod.yandex.net --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **release to MYT all but myt1** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/pre-prod/iam-access-service/update/allaz.yaml --filter "@ru-central1-c -iam-as-myt1.svc.cloud-preprod.yandex.net" --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **release to SAS, VLA** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/pre-prod/iam-access-service/update/allaz.yaml --filter "@ru-central1-a @ru-central1-b" --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

{% endcut %}

Следим за [сервисами](https://grafana.yandex-team.ru/d/iam-duty-deploy/iam-duty-deploy?orgId=1&from=now-3h&to=now&refresh=1m&var-appFilter=*_server&var-ds=Solomon%20Cloud%20Preprod&var-cluster=preprod&var-host=All) и за [выкатываемыми нодами](https://grafana.yandex-team.ru/d/iam-duty_dd-grpc-by-host/iam-grpc-by-hosts?orgId=1&from=now-3h&to=now&var-service=access-service&var-ds=Solomon%20Cloud%20Preprod&var-cluster=preprod&var-host=All&var-app=access-service_server&var-method=All&refresh=1m).

* Update common:
    1. На одной машине:
        ```bash
        yc-bootstrap --apply --template workflows/pre-prod/common/update/iam-access-service.yaml  --filter host=iam-as-myt1.svc.cloud-preprod.yandex.net --ticket-id ${TICKET_ID}
        ```
    1. На всех:
        ```bash
        yc-bootstrap --apply --template workflows/pre-prod/common/update/iam-access-service.yaml  --filter "%iam-access-service -iam-as-myt1.svc.cloud-preprod.yandex.net" --ticket-id ${TICKET_ID}
        ```
* Prepare (move-release):
    ```bash
    yc-bootstrap --apply --template workflows/pre-prod/iam-access-service/move-base-release/from-testing.yaml --ticket-id ${TICKET_ID}
    ```
* Update access-service:
    1. На одной машине:
        ```bash
        yc-bootstrap --apply --template workflows/pre-prod/iam-access-service/update/allaz.yaml --filter host=iam-as-myt1.svc.cloud-preprod.yandex.net --ticket-id ${TICKET_ID}
        ```
    1. В зоне myt:
        ```bash
        yc-bootstrap --apply --template workflows/pre-prod/iam-access-service/update/allaz.yaml --filter "@ru-central1-c -iam-as-myt1.svc.cloud-preprod.yandex.net" --ticket-id ${TICKET_ID}
        ```
    1. На остальных зонах:
        ```bash
        yc-bootstrap --apply --template workflows/pre-prod/iam-access-service/update/allaz.yaml --filter "@ru-central1-a @ru-central1-b" --ticket-id ${TICKET_ID}
        ```


# prod phase

## Internal-Prod
Downtime [устанавливаем](https://juggler.yandex-team.ru/downtimes/) для `host=yc_iam_svm_internal-prod`

{% cut "Шаблон процесса деплоя" %}

    ===[Internal-Prod] Access Service (<salt-formula-ver>:<yc-access-service-ver>)

    **iam-ya-myt1** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-ya-access-service/update/iam-ya.yaml --filter host=iam-ya-myt1.svc.cloud.yandex.net --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-ya-sas1** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-ya-access-service/update/iam-ya.yaml --filter host=iam-ya-sas1.svc.cloud.yandex.net --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-ya-vla1** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-ya-access-service/update/iam-ya.yaml --filter host=iam-ya-vla1.svc.cloud.yandex.net --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

{% endcut %}

Следим за [сервисами](https://grafana.yandex-team.ru/d/iam-duty-deploy/iam-duty-deploy?orgId=1&from=now-3h&to=now&refresh=1m&var-service=All&var-appFilter=*_server&var-ds=Solomon%20Cloud&var-cluster=prod-internal&var-host=All&var-service=access-service&var-service=iam-control-plane&var-service=openid-server&var-service=rm-control-plane&var-service=resource-manager&var-service=access-service) и за [выкатываемыми нодами](https://grafana.yandex-team.ru/d/iam-duty_dd-grpc-by-host/iam-grpc-by-hosts?orgId=1&from=now-3h&to=now&var-service=access-service&var-ds=Solomon%20Cloud&var-cluster=prod-internal&var-host=All&var-app=access-service_server&var-method=All&refresh=1m).

* Prepare (move-release):
  https://bb.yandex-team.ru/projects/CLOUD/repos/cluster-configs/browse/prod/salt-role-releases/iam-ya-access-service.yaml
* Update access-service:
    1. На iam-ya-myt1:
        ```bash
        # на ноду
        yc-bootstrap --apply --template workflows/prod/iam-ya-access-service/update/iam-ya.yaml --filter host=iam-ya-myt1.svc.cloud.yandex.net --ticket-id ${TICKET_ID}
        ```
    1. На iam-ya-sas1:
        ```bash
        # на ноду
        yc-bootstrap --apply --template workflows/prod/iam-ya-access-service/update/iam-ya.yaml --filter host=iam-ya-sas1.svc.cloud.yandex.net --ticket-id ${TICKET_ID}
        ```
    1. На iam-ya-vla1:
        ```bash
        # на ноду
        yc-bootstrap --apply --template workflows/prod/iam-ya-access-service/update/iam-ya.yaml --filter host=iam-ya-vla1.svc.cloud.yandex.net --ticket-id ${TICKET_ID}
        ```



## Prod
Downtime [устанавливаем](https://juggler.yandex-team.ru/downtimes/)
* PROD MYT1 `host=iam-as-myt1.svc.cloud.yandex.net`
* Israel IL1-A1 `host=iam-as-il1-a1.svc.yandexcloud.co.il`
* PROD ALL `host=yc_iam_as_svm_prod`
* Israel ALL `host=yc_iam_as_svm_israel`

{% cut "Шаблон процесса деплоя" %}

    ===[Prod] Access Service (<salt-formula-ver>:<yc-access-service-ver>)

    **iam-common iam-as-myt1** <{ TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/common/update/iam-access-service.yaml \
    --filter "host=iam-as-myt1.svc.cloud.yandex.net" --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-common il1-a1** <{ TODO
    ```bash
    yc-bootstrap --apply --template workflows/israel/common/update/iam-access-service.yaml \
    --filter "host=iam-as-il1-a1.svc.yandexcloud.co.il" --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-common-myt-other** <{ TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/common/update/iam-access-service.yaml \
    --filter "%iam-access-service @ru-central1-c -iam-as-myt1.svc.cloud.yandex.net" --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-common-il1-other** <{ TODO
    ```bash
    yc-bootstrap --apply --template workflows/israel/common/update/iam-access-service.yaml \
    --filter "%iam-access-service -iam-as-il1-a1.svc.yandexcloud.co.il" --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-common-sas** <{ TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/common/update/iam-access-service.yaml  --filter %iam-access-service --filter @ru-central1-b --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-common-vla** <{ TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/common/update/iam-access-service.yaml  --filter %iam-access-service --filter @ru-central1-a --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **move release (PROD)** <{ TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/move-base-release/from-pre-prod.yaml --ticket-id ${TICKET_ID}
    ```
    <[
    ]>
    }>

    **move release (Israel)** <{ TODO
    ```bash
    yc-bootstrap --apply --template workflows/israel/iam-access-service/move-base-release/from-prod.yaml --ticket-id ${TICKET_ID}
    ```
    <[
    ]>
    }>

    **iam-as-myt1** <{ TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter host=iam-as-myt1.svc.cloud.yandex.net --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-il1-a1** <{ TODO
    ```bash
    yc-bootstrap --apply --template workflows/israel/iam-access-service/update/allaz.yaml --filter host=iam-as-il1-a1.svc.yandexcloud.co.il --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-s3-myt1** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter host=iam-as-s3-myt1.svc.cloud.yandex.net --ticket-id  ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-s3-myt2** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter host=iam-as-s3-myt2.svc.cloud.yandex.net --ticket-id  ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-s3-myt3** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter host=iam-as-s3-myt3.svc.cloud.yandex.net --ticket-id  ${TICKET_ID}
    ```
    <[

    ]>
    }>


    **iam-as-myt all** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter "%iam-access-service @ru-central1-c -iam-as-myt1.svc.cloud.yandex.net -iam-as-s3-myt1.svc.cloud.yandex.net -iam-as-s3-myt2.svc.cloud.yandex.net -iam-as-s3-myt3.svc.cloud.yandex.net" --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-il1 all** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/isreael/iam-access-service/update/allaz.yaml --filter "%iam-access-service -iam-as-il1-a1.svc.yandexcloud.co.il" --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-s3-vla1** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter host=iam-as-s3-vla1.svc.cloud.yandex.net --ticket-id  ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-s3-vla2** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter host=iam-as-s3-vla2.svc.cloud.yandex.net --ticket-id  ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-s3-vla3** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter host=iam-as-s3-vla3.svc.cloud.yandex.net --ticket-id  ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-vla all** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter "%iam-access-service @ru-central1-a -iam-as-s3-vla1.svc.cloud.yandex.net -iam-as-s3-vla2.svc.cloud.yandex.net -iam-as-s3-vla3.svc.cloud.yandex.net" --ticket-id ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-s3-sas1** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter host=iam-as-s3-sas1.svc.cloud.yandex.net --ticket-id  ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-s3-sas2** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter host=iam-as-s3-sas2.svc.cloud.yandex.net --ticket-id  ${TICKET_ID}
    ```
    <[

    ]>
    }>

    **iam-as-s3-sas3** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter host=iam-as-s3-sas3.svc.cloud.yandex.net --ticket-id  ${TICKET_ID}
    ```
    <[

    ]>
    }>


    **iam-as-sas all** <{TODO
    ```bash
    yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter "%iam-access-service @ru-central1-b -iam-as-s3-sas1.svc.cloud.yandex.net -iam-as-s3-sas2.svc.cloud.yandex.net -iam-as-s3-sas3.svc.cloud.yandex.net" --ticket-id  ${TICKET_ID}
    ```
    <[

    ]>
    }>

{% endcut %}

Следим за сервисами:
* [PROD](https://grafana.yandex-team.ru/d/iam-duty-deploy/iam-duty-deploy?orgId=1&from=now-3h&to=now&refresh=1m&var-appFilter=*_server&var-ds=Solomon%20Cloud&var-cluster=prod&var-host=All)
* [Israel](https://grafana.yandex-team.ru/d/iam-duty-deploy/iam-duty-deploy?orgId=1&from=now-3h&to=now&refresh=1m&var-appFilter=*_server&var-ds=Solomon%20Cloud%20IL&var-cluster=israel&var-host=All&var-service=All)

Следим за выкатываемыми нодами
* [PROD](https://grafana.yandex-team.ru/d/iam-duty_dd-grpc-by-host/iam-grpc-by-hosts?orgId=1&from=now-3h&to=now&var-service=access-service&var-ds=Solomon%20Cloud&var-cluster=prod&var-host=All&var-app=access-service_server&var-method=All&refresh=1m)
* [Israel] (https://grafana.yandex-team.ru/d/iam-duty_dd-grpc-by-host/iam-grpc-by-hosts?orgId=1&from=now-3h&to=now&var-service=access-service&var-ds=Solomon%20Cloud%20IL&var-cluster=israel&var-host=All&var-app=access-service_server&var-method=All&refresh=1m)

На графике загрузки кешей [PROD](https://solomon.cloud.yandex-team.ru/?project=yc.iam.service-cloud&cluster=access-service-prod&service=access-service&l.sensor=guava_cache_max_loading_attempts&l.cache=ResourcePermissionsCache&graph=auto&l.host=iam-as-s3-*&stack=false), [Israel](https://solomon.yandexcloud.co.il/?project=yc.iam.service-cloud&cluster=access-service-israel&service=access-service&l.sensor=guava_cache_max_loading_attempts&l.cache=ResourcePermissionsCache&graph=auto&l.host=*&stack=false) не должно быть полок (могут быть пики, но не полки).

* Update common:
    1. На одной машине:
        ```bash
        yc-bootstrap --apply --template workflows/prod/common/update/iam-access-service.yaml  --filter host=iam-as-myt1.svc.cloud.yandex.net --ticket-id ${TICKET_ID}
        ```
    1. На всех (PROD):
        ```bash
        yc-bootstrap --apply --template workflows/prod/common/update/iam-access-service.yaml  --filter "%iam-access-service" --ticket-id ${TICKET_ID}
        ```
    1. На всех (Israel):
        ```bash
        yc-bootstrap --apply --template workflows/israel/common/update/iam-access-service.yaml  --filter "%iam-access-service" --ticket-id ${TICKET_ID}
        ```
* Move release (PROD):
    ```(bash)
    yc-bootstrap --apply --template workflows/prod/iam-access-service/move-base-release/from-pre-prod.yaml --ticket-id ${TICKET_ID}
    ```
* Move release (Israel):
    ```(bash)
    yc-bootstrap --apply --template workflows/israel/iam-access-service/move-base-release/from-prod.yaml --ticket-id ${TICKET_ID}
    ```
* Update access-service
    1. На одной машине:
        ```(bash)
        yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter host=iam-as-myt1.svc.cloud.yandex.net --ticket-id ${TICKET_ID}
        ```
    1. На зоне myt ([ru-central1-c](https://bb.yandex-team.ru/projects/CLOUD/repos/bootstrap-templates/browse/rackmap/az.yaml)):
        ```(bash)
        yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter "%iam-access-service @ru-central1-c -iam-as-myt1.svc.cloud.yandex.net" --ticket-id ${TICKET_ID}
        ```
    1. Финально на зонах sas ([ru-central1-b](https://bb.yandex-team.ru/projects/CLOUD/repos/bootstrap-templates/browse/rackmap/az.yaml)) и vla ([ru-central1-a](https://bb.yandex-team.ru/projects/CLOUD/repos/bootstrap-templates/browse/rackmap/az.yaml)):
        ```bash
        yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter %iam-access-service @ru-central1-b --ticket-id ${TICKET_ID}
        ```
        ```bash
        yc-bootstrap --apply --template workflows/prod/iam-access-service/update/allaz.yaml --filter %iam-access-service @ru-central1-a --ticket-id ${TICKET_ID}
        ```
    1. Israel:
        ```bash
        yc-bootstrap --apply --template workflows/israel/iam-access-service/update/allaz.yaml --filter %iam-access-service --ticket-id ${TICKET_ID}
        ```
