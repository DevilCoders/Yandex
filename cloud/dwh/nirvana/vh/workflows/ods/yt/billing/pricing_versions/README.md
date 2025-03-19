#### SKU pricing versions:

Разворачивает json-поле pricing_versions из таблицы skus в отдельную таблицу

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/pricing_versions)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/pricing_versions)
/ [PROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod_internal/ods/billing/pricing_versions)
/ [PREPROD_INTERNAL](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod_internal/ods/billing/pricing_versions)

* `sku_id` - `id` скушки
* `pricing_unit` - тип единицы потребления для подсчета стоимости
* `start_time` - начало действия этой версии
* `end_time` - конец действия
* `start_pricing_quantity` - левая граница потребления в единицах `pricing_unit`
* `end_pricing_quantity` - правая граница потребления в единицах `pricing_unit`
* `unit_price` - цена за единицу `pricing_unit`
* `aggregation_info_level` - интервал агрегации для порогов `pricing_quantity`
* `aggregation_info_interval` - сущность, для которой агрегируется
