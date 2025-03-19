## yandexcloud_cpu_utilization
#solomon #cpu-util #yandexcloud

Метрики cpu-util из Solomon. Данные загружены с 2020-01-01 по 2022-04-12

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                                       | Источники                                                                                                                                      |
|---------|-------------------------------------------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [cpu-util](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/solomon/yandexcloud_cpu_utilization/cpu-util)    | [SOLOMON_CPU_UTILIZATION](https://solomon.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_compute&service=compute&l.metric=cpu-util)    |
| PREPROD | [cpu-util](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/solomon/yandexcloud_cpu_utilization/cpu-util) | [SOLOMON_CPU_UTILIZATION](https://solomon.yandex-team.ru/?project=yandexcloud&cluster=cloud_preprod_compute&service=compute&l.metric=cpu-util) |


### Структура
| Поле        | Описание               |
|-------------|------------------------|
| _meta       | мета информация        |
| cloud_id    | идентификатор облака   |
| cluster     | идентификатор кластера |
| folder_id   | идентификатор папки    |
| host        | идентификатор хоста    |
| instance_id | идентификатор инстанса |
| metric      | название метрики       |
| project     | название проекта       |
| service     | название сервиса       |
| timestamp   | timestamp              |
| value       | значение метрики       |


### Загрузка

Статус загрузки:  не активна

Периодичность загрузки: разовая.
