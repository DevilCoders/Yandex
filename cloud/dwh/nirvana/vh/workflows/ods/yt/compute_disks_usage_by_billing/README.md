#### Compute disks usage:

Генерирует дневные таблицы с дисками NBS на каждый DC по данным биллинга. Нужно для расчета capacity по дискам.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud_analytics/dwh/ods/compute/disks)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud_analytics/dwh_preprod/ods/compute/disks)

* `billing_account_id` - id биллинг аккаунта
* `billing_account_state` - состояние биллинг акаунта
* `billing_account_status` - статус биллинг акаунта
* `date` - дата в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow`
* `hour` - дата с округлением до начала часа в iso-формате `%Y-%m-%dT%H:00:00` (например, `2021-12-31T23:00:00`) в таймзоне `Europe/Moscow`
* `disk_id` - id диска
* `disk_size` - размер диска
* `disk_size` - размер диска в байтах
* `disk_type` - тип диска
* `cloud_id` - id облака
* `datacenter` - ДЦ
