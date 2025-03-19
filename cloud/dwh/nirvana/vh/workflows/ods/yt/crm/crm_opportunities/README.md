## crm opportunities
#crm #crm_opportunities

Содержит информацию о сделке.


#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных

| Контур  | Расположение данных                                                                                                      | Источники                                                                                                                                   |
|---------|--------------------------------------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [crm_opportunities](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_opportunities) | [raw-crm_opportunities](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_opportunities) |
| PREPROD | [crm_opportunities](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/crm/crm_opportunities) | [raw-crm_opportunities](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/raw/mysql/crm-cloud/cloud8_opportunities) |


### Структура

| Поле                         | Описание                                                                                                                         |
|------------------------------|----------------------------------------------------------------------------------------------------------------------------------|
| acl_crm_team_set_id          |                                                                                                                                  |
| amount                       | объем сделки (месячный runrate)                                                                                                  |
| amount_usdollar              | объем сделки (месячный runrate) в базовой валюте (знаю, не логично)                                                              |
| assigned_user_id             | присвоенный id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) , `FK`       |
| ba_base_rate                 | rate валюты из связанного ba                                                                                                     |
| ba_currency_id               | id валюты из связанного ba                                                                                                       |
| base_rate                    | rate валюты сделки                                                                                                               |
| baseline_trend               | тренд (%) для baseline-потребления                                                                                               |
| best_case                    | лучший likely                                                                                                                    |
| block_reason                 | причина блокировки                                                                                                               |
| crm_campaign_id              | id компании                                                                                                                      |
| closed_revenue_line_items    |                                                                                                                                  |
| commit_stage                 | подтвержденная стадия сделки                                                                                                     |
| consumption_type             | тип потребления                                                                                                                  |
| created_by                   | созданный                                                                                                                        |
| crm_currency_id              | id валюты                                                                                                                        |
| date_closed                  | дата закрытия                                                                                                                    |
| date_closed_by               | тип даты закрытия (дата или месяц?)                                                                                              |
| date_closed_month            | месяц закрытия                                                                                                                   |
| date_closed_timestamp        | дата время закрытия                                                                                                              |
| date_entered_ts              | дата время ввода, utc                                                                                                            |
| date_entered_dttm_local      | дата время ввода, local tz                                                                                                       |
| date_modified_ts             | дата время модификации, utc                                                                                                      |
| date_modified_dttm_local     | дата время модификацииа, local tz                                                                                                |
| deleted                      | удалена ли сделка?                                                                                                               |
| crm_opportunity_description  | описание                                                                                                                         |
| discount                     | скидка                                                                                                                           |
| display_status               | выводимый статус                                                                                                                 |
| dri_workflow_template_id     | id шаблона workflow                                                                                                              |
| first_month_consumption      | месяц первого потребления                                                                                                        |
| first_trial_consumption_date | дата первого потребления                                                                                                         |
| forecast                     | участвует ли в прогнозе?                                                                                                         |
| full_capacity_date           | дата выхода на полный объем в рамках сделки                                                                                      |
| crm_opportunity_id           | id сделки, `PK`                                                                                                                  |
| included_revenue_line_items  | количество подтвержденных связанных RLI                                                                                          |
| initial_capacity             | стартовый объем сделки                                                                                                           |
| lead_source                  | источник лида, из которого создана сделка                                                                                        |
| lead_source_description      | описание источника лида, из которого создана сделка                                                                              |
| likely_daily                 | дневной объем сделки (дневной runrate)                                                                                           |
| likely_formula               | формула для объема сделки (amount)                                                                                               |
| linked_total_calls           | кол-во связанных звонков                                                                                                         |
| linked_total_notes           | кол-во связанных заметок                                                                                                         |
| linked_total_quotes          | количество связанных квот                                                                                                        |
| linked_total_tasks           | кол-во связанных задача                                                                                                          |
| lost_reason                  | причина проигрыша                                                                                                                |
| lost_reason_description      | описание причины проигрыша                                                                                                       |
| modified_user_id             | id [пользователя](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_users) внесшего изменение, `FK` |
| crm_opportunity_name         | название                                                                                                                         |
| next_step                    | следующий шаг                                                                                                                    |
| non_recurring                | в рамках сделки есть одномоментное потребление?                                                                                  |
| old_format_dimensions        | присутствует измерения старого формата?                                                                                          |
| opportunity_type             | тип                                                                                                                              |
| oppty_id                     | id сделки                                                                                                                        |
| paid_consumption             | оплачиваемое потребление                                                                                                         |
| partner_id                   | id партнера                                                                                                                      |
| partners_value               | польза от партнера (перепродажа и/или внедрение)                                                                                 |
| person_type                  | тип                                                                                                                              |
| probability                  | вероятность (%) (корректные данные)                                                                                              |
| probability_enum             | вероятность (список)                                                                                                             |
| probability_new              | вероятность (%) (некорректные данные)                                                                                            |
| revenue_ordinated            | кто сгенеририровал сделку (мы, партнер)?                                                                                         |
| sales_stage                  | этап продажи                                                                                                                     |
| sales_status                 | статус продажи                                                                                                                   |
| scenario                     | сценарий для сделки                                                                                                              |
| segment_ba                   | сегмент связанного биллинг-аккаунта                                                                                              |
| service_type                 | тип сервиса                                                                                                                      |
| services_total               | сумма оказываемых услуг                                                                                                          |
| services_total_usdollar      | сумма оказываемых услуг в базовой валюте                                                                                         |
| tax_rate_id                  | id налоговой ставки                                                                                                              |
| tax_rate_value               | значение налоговой ставки                                                                                                        |
| crm_team_id                  | id [primary_team](https://a.yandex-team.ru/arc_vcs/cloud/dwh/nirvana/vh/workflows/ods/yt/crm/crm_teams), `FK`                    |
| crm_team_set_id              | id team_set (мб связано несколько команд)                                                                                        |
| total_revenue_line_items     | общее количество связанных RLI                                                                                                   |
| tracker_number               | номер в трекере                                                                                                                  |
| trial_consumption            | триальное потребление                                                                                                            |
| worst_case                   | худший likely (прогноз)                                                                                                          |


### Структура
Статус загрузки: активна

Периодичность загрузки: 1h.
