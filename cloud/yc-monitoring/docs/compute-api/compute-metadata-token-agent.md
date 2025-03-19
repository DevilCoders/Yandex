[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+metadata+token-agent), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-metadata-token-agent)

## Описание
Горит, когда сервисный токен не обновлен в течение минуты после расчетного времени обновления.

## Подробности
Возможные причины:
1. Проблемы запроса токена у токен-агента
2. Приходит "несвежий" токен
3. Проблема в реализации

## Диагностика
- Смотрим на [дашборде](https://grafana.yandex-team.ru/d/TLq5-QSMk/cloud-compute-metadata-service-per-host?orgId=1) проблемы с токен-сервисом
- Если в логах находим `Error receiving token {for instance_id}: ... ` - у нас *1 вариант* -> Идём к дежурным **IAM**
- Если находим `Received token {for instance_id} (<token CRC>), should have been refreshed <time> ago` - токен еще не просрочен, но момент обновления прошел, либо `Received token {for instance_id} (<token CRC>), EXPIRED <time> ago` - пришёл просроченный токен. Это *2 вариант* -> так же идём к дежурным **IAM**
- В случае, когда ошибка возникла в самом сервисе, рекомендуется обратиться к майнтейнеру сервиса/авторам последних изменений в пакете `common/pkg/auth` или откатить сервис на предыдущую стабильную версию: `apt install --allow-downgrades yc-compute-metadata=...`
