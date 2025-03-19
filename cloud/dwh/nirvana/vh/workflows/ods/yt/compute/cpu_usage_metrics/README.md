## Compute CPU Usage Metrics
#compute #cpu #cpu_metrics

Метрики потребления CPU в compute.

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)
4. [Пояснения](#пояснения)


### Расположение данных
| Контур    | Расположение данных   | Источники |
| --------- | -------------------   | --------- |
| PROD      | [cpu_usage_metrics](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/compute/cpu_usage_metrics)    | [lbk-compute-cpu-metrics](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/logbroker/compute/cpu-usage/yc-dwh_mirrored_transfer_prod_compute_lbk-compute-cpu-metrics)     |
| PREPROD   | [cpu_usage_metrics](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/compute/cpu_usage_metrics) | [lbk-compute-cpu-metrics](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/logbroker/compute/cpu-usage/yc-dwh_mirrored_transfer_preprod_compute_lbk-compute-cpu-metrics)  |


### Структура
| Поле              | Описание                                 |
| ----------------- | ---------------------------------------- |
| metric_ts         | Timestamp метрики                        |
| metric_dttm_local | Datetime метрики в часовом поясе МСК     |
| metric_value      | Значение метрики, измеряется в процентах: какой процент все ядра VM занимают от _гарантированной емкости_. Значение может быть больше 100 - это _повезло_ [берсту](#burst).                   |
| cloud_id          | ID [облака](../../iam/clouds)            |
| folder_id         | ID [каталога](../../iam/folders)         |
| metric_name       | Наименование метрики                     |
| resource_id       | ID ресурса                               |
| resource_type     | Тип ресурса                              |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.

### Пояснения
#### Burst 
Burst - это модель, когда клиент может забронировать какую-то долю ядер на общем пуле. 
Например, можно взять 25% ядра. Это значит, что остальные 75% будут распределены между другими клиентами. Но бывает как, что сосед не пользуется своей долей. Тогда клиент, купивший 25% ядра, может получить целое ядро в распоряжение. А целое ядро - это 400% от запланированного
