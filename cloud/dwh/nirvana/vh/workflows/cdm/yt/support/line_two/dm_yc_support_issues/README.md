#### YC Support Issues

Витрина тикетов YC Support осереди CLOUDLINETWO.

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/cdm/support/line_two/dm_yc_support_issues)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/cdm/support/line_two/dm_yc_support_issues)

- `author_id` - ID [пользователя](../../../../../ods/yt/startrek/common/users), который завел тикет.
- `created_at` - Дата и время заведения тикета.
- `resolved_at` - Дата и время решения тикета.
- `issue_key` - Код тикета в startrek.
- `issue_id` - ID [тикета](../../../../../ods/yt/startrek/cloud_line_two/issues) в startrek.
- `issue_type` - [Тип](../../../../../ods/yt/startrek/common/types) тикета.
- `payment_tariff` - Тариф инициатора тикета.
- `billing_account_id` - ID [billing_account](../../../../../ods/yt/billing/billing_accounts)
- `feedback_reaction_speed` - Обратная связь, оценка скорости реакции.
- `feedback_response_completeness` - Обратная связь, оценка полноты ответа.
- `feedback_general` - Обратная связь, оценка поддержки в целом.
- `components_quotas`- Содержится ли [компонент](../../../../../ods/yt/startrek/cloud_line_two/components) 'квоты' среди компонентов, указанных в тикете.

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
- `moved_to_linetwo_queue_at` - Дата и время, когда тикет был переведен в очередь CLOUDLINETWO. Если переводов было несколько, здесь будет дата и время последнего.
- `linetwo_spent_ms` - Количество времени, которое тикет находился в очереди CLOUDLINETWO. Рассчитывается как разница между `resolved_at` и `moved_to_linetwo_queue_at`.

