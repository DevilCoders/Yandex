[Алерт vpc-node-sync-messages-context-error в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dvpc-node-sync-messages-context-error)

## Что проверяет

Ошибки отмены контекста при обработке запросов акторами. Горит **красным**, если за последние десять минут ошибок больше 15. **Жёлтым**, если больше 5.

## Если загорелось

- посмотреть метрику `message_sync_ctx_error_count` для детализации типа запросов и типа ошибок

- смотреть логи сервиса