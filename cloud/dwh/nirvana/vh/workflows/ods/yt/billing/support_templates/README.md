## Support templates
#ods #billing #support_templates

Вычитывает последний (актуальный) снапшот таблицы (`support_templates`)

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                             | Источники                                                                                                                                                             |
|---------|---------------------------------------------------------------------------------------------------------------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [support_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/billing/support_templates)    | [raw-support_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/ydb/billing/hardware/default/billing/meta/support_templates)    |
| PREPROD | [support_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/billing/support_templates) | [raw-support_templates](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/raw/ydb/billing/hardware/default/billing/meta/support_templates) |


### Структура
| Поле                          | Описание                        |
|-------------------------------|---------------------------------|
| created_ts                    | дата время создания, utc        |
| created_dttm_local            | дата время создания, local tz   |
| fixed_consumption_schema      | фиксированная схема потребления |
| support_template_id           | id шаблона поддержки, `PK`      |
| support_template_name         | название шаблона поддержки      |
| is_subscription_allowed       | разрешена ли подписка?          |
| percentage_consumption_schema | схема процентного потребления   |
| priority_level                | уроень приоритета               |
| state                         | состояние                       |
| support_template_name_en      | Название,en                     |
| support_template_name_ru      | Название,ru                     |
| updated_ts                    | дата время обновления, utc      |
| updated_dttm_local            | дата время обновления, local tz |

### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
