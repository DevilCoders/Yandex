## Агрегированные данные в разрезе sku_service_name для экспорта
#cloud_analytics #yc_consumption #sku_service_name #export

Агрегированные данные в разрезе crm_segment для экспорта

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур    | Расположение данных                                                                                                                                       | Источники                                                                                                               |
| --------- |-----------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------|
| PROD      | [dm_yc_consumption_by_sku_service_name](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/export/yandex-analytics/dm_yc_consumption_by_sku_service_name)    | [dm_yc_consumption](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_yc_consumption)    |
| PREPROD   | [dm_yc_consumption_by_sku_service_name](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/export/yandex-analytics/dm_yc_consumption_by_sku_service_name) | [dm_yc_consumption](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_yc_consumption) |


### Структура
| Поле                      | Описание                                                                                      |
|---------------------------|-----------------------------------------------------------------------------------------------|
| billing_record_msk_date   | дата потребления в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow` |
| sku_service_name          | название сервиса SKU                                                                          |
| unique_billing_account_id | количество уникальных billing_account_id                                                      |





### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
