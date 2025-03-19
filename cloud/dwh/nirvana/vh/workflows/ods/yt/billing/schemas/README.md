## Var adjustments
#ods #billing #schemas

Вычитывает последний (актуальный) снапшот таблицы (`schemas`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                         | Источники                                                                                                                                         |
|---------|-------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [schemas](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/schemas)    | [raw-schemas](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/schemas)    |
| PREPROD | [schemas](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/schemas) | [raw-schemas](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/schemas) |


### Структура
| Поле        | Описание                         |
|-------------|----------------------------------|
| schema_name | название схемы                   |
| service_id  | id сервиса                       |
| tag_level   | уровень тэга: optional, required |
| tag_name    | название тэга                    |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
