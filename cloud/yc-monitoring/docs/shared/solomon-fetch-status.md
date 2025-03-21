[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=solomon+agent+)

## Описание
Горит, если есть проблемы со сбором данных в Соломон.

## Подробности
На хостах соломон-агент формирует ответ для Соломона. Fetcher идет в соломон-агент согласно конфигурации сервиса.
Успешность получения данных (или ошибка) фиксируется в метрике, на которую смотрит алерт. Что может пойти не так:
- слишком большой ответ
- кончилась квота
- слишком много метрик
- ... и много другое.

## Диагностика
- Алерт загорается на конкретный шард (project|service)
- зайти на страницу статуса шарда `https://solomon.yandex-team.ru/admin/projects/yandexcloud/shards/<shard ID>/status`:
    * вкладка `Targets` показывает список хостов (источников данных для шарда) и статус получения данных с каждого хоста (можно увидеть конкретную ошибку).
    * вкладка `Fetching` содержит графики успешности опроса хостов, на которой можно посмотреть статус ошибки и то когда она началась (можно отследить динамику проблемы).
    * вкладка `Metric limits` отображает данные об испольновании квоты на кол-во метрик шардом (необходимо, чтобы понять на сколько необходимо поднять квоту).
- найти проблему в [списке](https://solomon.yandex-team.ru/docs/faq) и следовать рекомендациям.
