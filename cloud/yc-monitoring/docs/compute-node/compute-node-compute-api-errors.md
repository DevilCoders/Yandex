[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=Critical+Compute+API+errors+on+Compute+Node), [Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-node-compute-api-errors)

## compute-node-compute-api-errors
Загорается на все критические ошибки при походе в Compute API. На данный момент это только ошибки валидации запросов/ответов.

## Подробности
Алерт делался в результате разъезда Compute API и Compute Node. Основной риск тут - что где-то модельки не сойдутся, и мы друг друга не поймем. Решили, что пятисотки и пр. пока будем мониторить на серверной стороне, а на стороне ноды - только действительно критичные ошибки валидации, которые не могут флапать. Алерт триггерится на любое количество ошибок - их быть не должно. Если загорелся - то определенно надо смотреть.

Помимо абстрактных ошибок валидации также включает в себя [ошибки устаревших ревизий](https://st.yandex-team.ru/CLOUD-81352).

## Диагностика
В алерте есть список API-методов, для которых он загорелся. Идем на ноду и читаем логи - ищем сообщения об ошибках при походе в Compute API.

Логи можно посмотреть командой `journalctl -u yc-compute-node -p1..3 --since -2h`

## Ссылки
- Возможная причина: [CLOUD-81352: Ревизии instance state в запросах из compute node в compute private API](https://st.yandex-team.ru/CLOUD-81352)
- Возможная причина: [CLOUD-90425: Из API может приехать невалидный инстанс с битыми моделями сетевых интерфейсов](https://st.yandex-team.ru/CLOUD-90425)
- [CLOUD-71928: Реализовать на стороне ноды мониторинг для клиента private API](https://st.yandex-team.ru/CLOUD-71928)

