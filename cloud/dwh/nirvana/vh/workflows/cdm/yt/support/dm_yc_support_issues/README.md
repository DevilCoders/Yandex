#### YC Support Issues

Витрина тикетов YC Support.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/support/dm_yc_support_issues)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/support/dm_yc_support_issues)

- `author_id` - ID [пользователя](../../../../ods/yt/startrek/common/users), который завел тикет.
- `created_at` - Дата и время заведения тикета.
- `resolved_at` - Дата и время решения тикета.
- `issue_key` - Код тикета в startrek.
- `issue_id` - ID [тикета](../../../../ods/yt/startrek/cloud_support/issues) в startrek.
- `issue_type` - [Тип](../../../../ods/yt/startrek/common/types) тикета.
- `payment_tariff` - Тариф инициатора тикета.
- `billing_account_id` - ID [billing_account](../../../../ods/yt/billing/billing_accounts)
- `feedback_reaction_speed` - Обратная связь, оценка скорости реакции.
- `feedback_response_completeness` - Обратная связь, оценка полноты ответа.
- `feedback_general` - Обратная связь, оценка поддержки в целом.
- `components_quotas`- Содержится ли [компонент](../../../../ods/yt/startrek/cloud_support/components) 'квоты' среди компонентов, указанных в тикете.

- `comment_sla_spent_total_ms` - Сумма длительностей всех таймеров [sla по комментариям](../dm_yc_support_comment_slas).
- `comment_sla_first_timer_stopped_at` - datetime первой остановки таймера [sla по комментариям](../dm_yc_support_comment_slas), МСК.
- `comment_sla_first_timer_stop_lag_ms` - Разница между временем заведения тикета (`created_at`) и первой остановкой таймера sla, ms.
- `comments_outgoing_cnt` - Количество исходящих сообщений.
- `comments_incoming_cnt` - Количество входящих сообщений.
- `comments_from_support_cnt` - Количество сообщений, оставленных сотрудниками поддержки.

- `has_summonee` - был ли хотя бы один призыв.
- `first_status_change_at` - Время первого изменения статуса.
- `crm_segment` - Сегмент пользователя, который завел тикет, на дату создания тикета.
- `segment_current` - Сегмент пользователя, который завел тикет, на сегодня.
- `summons_total_cnt` - Общее количество человек, призванных в тикет.
- `summons_second_line_cnt` - Количество сотрудников второй линии поддержки, призванных в тикет.
- `summons_support_cnt` - Количество сотрудников поддержки, призванных в тикет.
- `summons_external_cnt` - Количество человек не из поддержки, призванных в тикет. Рассчитывается как `summons_total_cnt` - `summons_support_cnt`.

