## Tracker CLOUDLINETWO issues:
#dm #support #line_two #startrek

SLA на комментарии в тикетах CLOUDLINETWO.

#### Ссылки
1. [Расположение данных.](#расположение-данных)
2. [Структура.](#структура)
3. [Загрузка.](#загрузка)


### Расположение данных
| Контур  | Расположение данных                                                                                                                                        |
|---------|------------------------------------------------------------------------------------------------------------------------------------------------------------|
| PROD    | [dm_yc_support_comment_slas](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/support/line_two/dm_yc_support_comment_slas)    |
| PREPROD | [dm_yc_support_comment_slas](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/support/line_two/dm_yc_support_comment_slas) |


### Структура
| Поле                  | Описание                                                                                                               |
|-----------------------|------------------------------------------------------------------------------------------------------------------------|
| author_id             | ID автора комментария [из трекера](../../../../../ods/yt/startrek/common/users).                                       |
| issue_id              | ID [issue](../../../../../ods/yt/startrek/cloud_line_two/issues)                                                       |
| created_at            | Дата и время создания комментария.                                                                                     |
| payment_tariff        | Тариф клиента, берется из свойств [тикета](../../../../../ods/yt/startrek/cloud_line_two/issues).                      |
| support_comment_id    | ID комментария [в саппорте](../../../../../ods/yt/support/comments)                                                    |
| direction             | Направление комментария: incoming (от пользователя поддержке) или outcoming (от поддержки пользователю).               |
| iam_user_id           | ID [пользователя в IAM](../../../../../ods/yt/iam/passport_users), оставившего комментарий.                            |
| timer_id              | ID настройки таймеры в startrek.                                                                                       |
| fail_at               | Дата и время, когда произошел fail. NULL, если fail'a не было.                                                         |
| fail_threshold_ms     | Порог до нарушения sla.                                                                                                |
| paused_duration_ms    | Время приостановки, мс.                                                                                                |
| spent_ms              | Времени затрачено, мс.                                                                                                 |
| started_at            | Дата и время старта таймера.                                                                                           |
| stopped_at            | Дата и время остановки таймера.                                                                                        |
| violation_status      | Статус, показывающий было ли нарушение sla.                                                                            |
| warn_at               | Дата и время, когда должно (было) произойти предупреждение.                                                            |
| warn_threshold_ms     | Порог до срабатывания предупреждения, мс.                                                                              |
| clock_status          | Статус таймера: _не запущен_ / _запущен_ / _остановлен_                                                                |
| response_interval     | Интервал времени ответа. Определяется на основе spent_ms.                                                              |
| staff_user_id         | ID автора комментария [из стаффа](../../../../../ods/yt/staff/persons).                                                |
| staff_department_name | Название департамента, в котором сотрудник непосредственно работает. [Из стаффа](../../../../../ods/yt/staff/persons). |


### Загрузка

Статус загрузки: активна

Периодичность загрузки: 1h.
