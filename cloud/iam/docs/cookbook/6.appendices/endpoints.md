При заказе дырок используйте в качестве destination адрес из колонки Endpoints.

## Access Service

[gRPC API](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/servicecontrol/v1/access_service.proto)

**Стенд** | **Endpoints** | **Примечание**
:--- | :--- | :---
PROD | as.private-api.cloud.yandex.net:4286 |
PRE-PROD | as.private-api.cloud-preprod.yandex.net:4286 |
TESTING | as.private-api.cloud-testing.yandex.net:4286 |
ISRAEL | as.private-api.yandexcloud.co.il:14286 |
INTERNAL-PROD | as.cloud.yandex-team.ru:4286 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_YA_PROD_NETS_:4286`
INTERNAL-PRESTABLE | as.prestable.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_PRESTABLE_NETS_:443`
INTERNAL-DEV | as.dev.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_DEV_NETS_:443`

## Token Service

[gRPC API](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/iam/v1/iam_token_service.proto)

**Стенд** | **Endpoints** | **Примечание**
:--- | :--- | :---
PROD | ts.private-api.cloud.yandex.net:4282 |
PRE-PROD | ts.private-api.cloud-preprod.yandex.net:4282 |
TESTING | ts.private-api.cloud-testing.yandex.net:4282 |
ISRAEL | ts.private-api.yandexcloud.co.il:14282 |
INTERNAL-PROD | ts.cloud.yandex-team.ru:4282 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_YA_PROD_NETS_:4282`
INTERNAL-PRESTABLE | ts.prestable.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_PRESTABLE_NETS_:443`
INTERNAL-DEV | ts.dev.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_DEV_NETS_:443`

## Resource Manager

[gRPC API](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/resourcemanager/v1/)

**Стенд** | **Endpoints** | **Примечание**
:--- | :--- | :---
PROD | rm.private-api.cloud.yandex.net:4284 |
PRE-PROD | rm.private-api.cloud-preprod.yandex.net:4284 |
TESTING | rm.private-api.cloud-testing.yandex.net:4284 |
ISRAEL | rm.private-api.yandexcloud.co.il:14284 |
INTERNAL-PROD | rm.cloud.yandex-team.ru:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_YA_PROD_NETS_:4284`
INTERNAL-PRESTABLE | rm.prestable.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_PRESTABLE_NETS_:443`
INTERNAL-DEV | rm.dev.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_DEV_NETS_:443`

## IAM

[gRPC API](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/iam/v1/)

**Стенд** | **Endpoints** | **Примечание**
:--- | :--- | :---
PROD | iam.private-api.cloud.yandex.net:4283 |
PRE-PROD | iam.private-api.cloud-preprod.yandex.net:4283 |
TESTING | iam.private-api.cloud-testing.yandex.net:4283 |
ISRAEL | iam.private-api.yandexcloud.co.il:14283 |
INTERNAL-PROD | iamcp.cloud.yandex-team.ru:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_YA_PROD_NETS_:4283`
INTERNAL-PRESTABLE | iam.prestable.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_PRESTABLE_NETS_:443`
INTERNAL-DEV | iam.dev.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_DEV_NETS_:443`

## MFA

[gRPC API](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/iam/v1/mfa/totp_profile_service.proto)

**Стенд** | **Endpoints** | **Примечание**
:--- | :--- | :---
PROD | mfa.private-api.cloud.yandex.net:6327 |
PRE-PROD | mfa.private-api.cloud-preprod.yandex.net:6327 |
TESTING | mfa.private-api.cloud-testing.yandex.net:6327 |
ISRAEL | mfa.private-api.yandexcloud.co.il:16327 |
INTERNAL-PROD | mfa.cloud.yandex-team.ru:443 | Дополнительно нужно заказывать доступ до _CLOUD_IAM_YA_PROD_NETS_:6327

## OAuth

[API](https://oauth.net/2/)

**Стенд** | **Endpoints** | **Примечание**
:--- | :--- | :---
PROD | auth.cloud.yandex.ru:443 |
PRE-PROD | auth-preprod.cloud.yandex.ru:443 | Для сетей дополнительно нужно заказывать доступ до `_CLOUD_L7_PREPROD_NETS_:2443`, доступ от людей уже есть на [staff cloud](https://staff.yandex-team.ru/departments/yandex_exp_9053)
TESTING | auth-testing.cloud.yandex.ru:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_PREPROD_NETS_:443`, доступ от людей уже есть на [staff cloud](https://puncher.yandex-team.ru/?id=6103d8abf9eebd64e83dabbf) & [YC Dev&Ops](https://puncher.yandex-team.ru/?id=5f058006a63d938e93526a47)
ISRAEL | auth.cloudil.co.il:443 |
INTERNAL-PROD | auth.cloud.yandex-team.ru:443 | Доступ от людей уже есть на [@allstaff@](https://puncher.yandex-team.ru/?id=5ea6cd48ea5e6fb33f1c88ab), для сетей дополнительно нужно заказывать доступ до __CLOUD_L7_PROD_NETS_:3443

## Session Service

[gRPC API](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/oauth/v1/session_service.proto)

**Стенд** | **Endpoints** | **Примечание**
:--- | :--- | :---
PROD | ss.private-api.cloud.yandex.net:8655 |
PRE-PROD | ss.private-api.cloud-preprod.yandex.net:8655 |
TESTING | ss.private-api.cloud-testing.yandex.net:8655 |
ISRAEL | ss.private-api.yandexcloud.co.il:18655 |
INTERNAL-PROD | ss.cloud.yandex-team.ru:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_YA_PROD_NETS_:8655`

## Identity (Legacy)

К этому бэкенду новых дырок заказывать не нужно. Если вам все еще требуется функциональность из него, можно использовать API Adapter или переключиться на один из новых бэкендов (IAM или Resource Manager).

**Стенд** | **Endpoints** | **Примечание**
:--- | :--- | :---
PROD | identity.private-api.cloud.yandex.net:14336 | ||
PRE-PROD | identity.private-api.cloud-preprod.yandex.net:14336 | ||
TESTING | identity.private-api.cloud-testing.yandex.net:14336 | ||
INTERNAL-PROD | iam.cloud.yandex-team.ru:443| Дополнительно нужно заказывать доступ до `_CLOUD_IAM_YA_PROD_NETS_:443` ||
INTERNAL-PRESTABLE | identity.prestable.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_PRESTABLE_NETS_:443`
INTERNAL-DEV | identity.dev.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_DEV_NETS_:443`

{% note info %}

Не делайте запросов в REST-endpoint ```https://iam.cloud.yandex-team.ru/v1/tokens``` для выписывания IAM-token.
Это REST-endpoint из legacy backend, он перестанет работать в ближайшее время.
Скорее всего вам нужен REST-вызов gateway https://gw.db.yandex-team.ru/iam/v1/tokens (либо gRPC вызов напрямую к token-service ```ts.cloud.yandex-team.ru:4282```).

{% endnote %}

## Yandex Team Integration

[gRPC API](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/team/integration/v1/)

**Стенд** | **Endpoints** | **Примечание**
:--- | :--- | :---
INTERNAL-PROD | ti.cloud.yandex-team.ru:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_YA_PROD_NETS_:4687`
INTERNAL-PRESTABLE | ti.prestable.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_PRESTABLE_NETS_:443`
INTERNAL-DEV | ti.dev.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_DEV_NETS_:443`

## Yandex Team Integration (IDM)

[REST API](https://wiki.yandex-team.ru/intranet/idm/api/)

**Стенд** | **Endpoints** | **Примечание**
:--- | :--- | :---
INTERNAL-PROD | idm.ti.cloud.yandex-team.ru:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_YA_PROD_NETS_:4367`

## Organization Service

[gRPC API](https://bb.yandex-team.ru/projects/CLOUD/repos/cloud-go/browse/private-api/yandex/cloud/priv/organizationmanager/v1/organization_service.proto)

**Стенд** | **Endpoints** | **Примечание**
:--- | :--- | :---
PROD | org.private-api.cloud.yandex.net:4290 |
PRE-PROD | org.private-api.cloud-preprod.yandex.net:4290 |
TESTING | org.private-api.cloud-testing.yandex.net:4290 |
ISRAEL | org.private-api.yandexcloud.co.il:14290 |
INTERNAL-PROD | org.cloud.yandex-team.ru:443| Дополнительно нужно заказывать доступ до `_CLOUD_IAM_YA_PROD_NETS_:443` ||
INTERNAL-PRESTABLE | org.prestable.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_PRESTABLE_NETS_:443`
INTERNAL-DEV | org.dev.cloud-internal.yandex.net:443 | Дополнительно нужно заказывать доступ до `_CLOUD_IAM_INTERNAL_DEV_NETS_:443`
