#### SKU overrides:

Персональные переопределения цен для клиентов

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/sku_overrides)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/sku_overrides)


* `billing_account_id` - идентификатор [платёжного аккаунта](../billing_accounts/README.md)
* `sku_id` - идентификатор [продукта](../skus/README.md)
* `start_time_msk` - момент начала действия переопределения цены
* `end_time_msk` - момент конца действия переопределения цены
* `rate_id` - порядковый номер порога потребления
* `start_pricing_quantity` - количество единиц потребления, с которого начинает действовать текущий ценовой порог
* `end_pricing_quantity` - количество единиц потребления, до которого действует текущий ценовой порог
* `unit_price` - цена за единицу потребления для текущего ценового порога
* `currency` - валюта поля `unit_price`
* `aggregation_info_interval` - интервал агрегации для порогов `pricing_quantity`
* `aggregation_info_level - сущность, для которой агрегируется
