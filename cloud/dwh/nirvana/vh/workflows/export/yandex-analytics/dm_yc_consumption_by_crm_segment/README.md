## Агрегированные данные в разрезе crm_segment для экспорта
#cloud_analytics #yc_consumption #crm_segment #export

Агрегированные данные в разрезе crm_segment для экспорта

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур    | Расположение данных                                                                                                                                       | Источники                                                                                                               |
| --------- |-----------------------------------------------------------------------------------------------------------------------------------------------------------|-------------------------------------------------------------------------------------------------------------------------|
| PROD      | [dm_yc_consumption_by_crm_segment](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/export/yandex-analytics/dm_yc_consumption_by_crm_segment)    | [dm_yc_consumption](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/dm_yc_consumption)    |
| PREPROD   | [dm_yc_consumption_by_crm_segment](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/export/yandex-analytics/dm_yc_consumption_by_crm_segment) | [dm_yc_consumption](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/dm_yc_consumption) |


### Структура
| Поле                         | Описание                                                                                      |
|------------------------------|-----------------------------------------------------------------------------------------------|
| crm_segment                  | CRM Segment                                                                                   |
| billing_record_msk_date      | дата потребления в iso-формате `%Y-%m-%d` (например, `2021-12-31`) в таймзоне `Europe/Moscow` |
| billing_record_total_rub_vat | итого к оплате с учетом скидок в рублях (cost + credit - reward), платное потребление без НДС |





### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.


