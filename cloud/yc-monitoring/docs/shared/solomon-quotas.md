[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=solomon+quotas)

## Описание
Проверяет квоту по количеству метрик в Solomon-шардах.

## Подробности
Загорается WARN, если использование более 95% либо в течение 7 дней упремся в квоту.
Загорается CRIT, если использование более 98% либо в течение 3 дней упремся в квоту.

Если уперлись в квоту, новые метрики Solomon отбрасывает. Новая метрика = новый набор labels.

## Диагностика
Переходим по ссылке "shard" из аннотаций алерта (на страницу шарда).

На странице шарда ([пример](https://solomon.yandex-team.ru/admin/projects/yandexcloud/shards/yandexcloud_cloud_prod_compute_compute_node)) нажимаем "Shard Status". Переходим на вкладку "Metric limits". Смотрим график кол-ва метрик и лимит.

![](https://jing.yandex-team.ru/files/simonov-d/solomon-metrics-limits.png)

Возможные пути решения:
- запросить у команды Solomon увеличение квоты (самый быстрый способ, но более 20 млн метрик обычно не дают) со страницы шарда,
- уменьшить TTL (старые метрики чистятся один раз в сутки),
- разбить один большой шард по AZ,
- писать меньше метрик (например, убрать сильно вариабельные метки, такие как instance_id).

![](https://jing.yandex-team.ru/files/simonov-d/solomon-shard-quotas-params.png)

## Полезные ссылки

- [Как работает TTL в Solomon](https://solomon.yandex-team.ru/docs/concepts/ttl)
- [Мониторинг квот в Solomon](https://solomon.yandex-team.ru/docs/best-practices/quota-monitoring)
- [Инструмент для анализа меток](https://solomon.yandex-team.ru/admin/projects/yandexcloud/metrics?selectors=cluster%3D%22cloud_prod_compute%22%2C+service%3D%22compute_node%22) - поможет ответить на вопрос "Какие метки (labels) сильнее всего отъедают квоту?"
