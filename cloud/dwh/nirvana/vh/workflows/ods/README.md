#### ODS. Operational Data Store

Операционные данные -- это таблицы из [RAW-слоя](../raw/README.md), которые прошли валидацию и были приведены к единому формату

#### Структура слоя

Данные в коде хранятся в `/ods/destination_system/entity_name`

* `destination_system` - место хранения данных, например `yt`
* `entity_name` - название сущности с данными

Данные в месте хранения должны лежать в `/ods/entity_name`

##### Таблицы

* [yt.compute_disks_usage_by_billing](./yt/compute_disks_usage_by_billing/README.md) - таблицы с дисками NBS.
* [yt.currency_rates](./yt/currency_rates/README.md) - курс валют.
* [yt.kikimr_disk_used_space](./yt/kikimr_disk_used_space/README.md) - потребление дисков KiKiMR.
* [yt.nbs_disk_purchased_space](./yt/nbs_disk_purchased_space/README.md) - зарезервированное потребление дисков NBS.
* [yt.nbs_disk_used_space](./yt/nbs_disk_used_space/README.md) - фактическое потребление дисков NBS.
* [yt.nbs_nrd_used_space](./yt/nbs_nrd_used_space/README.md) - потребление нереплицируемых дисков NBS.
* [yt.billing.billing_accounts](./yt/billing/billing_accounts/README.md) - биллинг аккаунты.
* [yt.billing.billing_accounts_history](./yt/billing/billing_accounts_history/README.md) - история биллинг аккаунтов.
* [yt.billing.publisher_accounts](./yt/billing/publisher_accounts/README.md) - аккаунты паблишеров.
* [yt.billing.skus](./yt/billing/skus/README.md) - продукты (SKU).
* [yt.cloud_analytics.sku_tags](./yt/cloud_analytics/sku_tags/README.md) - разметка SKU (теги).
