# grpc-restricted
[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?project=&query=service%3Dgrpc-restricted)

## Проверяет
- Контроллеры балансера (lb, hc, hc-proxy) находятся в restricted режиме, в котором на многие мутирующие gprc-операции возвращается ошибка status.Error(codes.Unavailable, "method manually restricted")

## Подробности
- Звонящий.
- Каждая мутирующая операция, которая была отклонена, увеличивает solomon-счетчик grpc_restricted на 1.
- Не все мутирующие операции будут блокироваться. Некоторые продолжают нормально функционировать. Например будут работать методы ```UpsertForcedStatuses```, ```UpdateTargetStatus```.
- Сервис работает по принципу белого списка. Если какой-то метод окажется ему неизвестным (см. ```isReadOnlyMethod``` в ```readonly_middleware.go```) то он будет заблокирован. Однако есть тест (```TestReadOnlyAPIMethods```), который проверяет, что все текущие grpc-методы зарегистрированны в ```readonly_middleware.go``` явным образом.
- Список grpc-методов на который оказывает влияние данный режим:
-- LB:```UpsertForwardingGroup```
-- LB:```DeleteForwardingGroup```
-- LB:```AddTargets```
-- LB:```RemoveTargets```
-- LB:```ReplaceForwardingGroupRules```
-- LB:```DeleteForwardingGroupRules```
-- HC:```Upsert```
-- HC:```Delete```
-- HCaaS:```Upsert```
-- HCaaS:```Delete```

## Что делать
- Аларм зажигается, если хост (как правило все хосты, т.к. нет большого смысла включать режим на отдельном хосте) были переведены дежурным в режим ```read-only-api```. Это делается посредством следующей команды c хоста контроллера:
```bash
$ curl -s 0x0:4050/debug/read-only-api?set=true
```
к указанной команде следует добавлять: ```&i-know-what-i-am-doing=true```, т.к. контекст её использования очень специфичен и выполнять её следует только понимая последствия от её применения.
Обычно в этом возникает необходимость при проведении каких-то CLOUDOPS или adhoc операций при работе с базой балансера. Например при создании или (частичном) восстановлении бекапа.
По окончании работ, контроллеры необходимо вернуть в рабочее состояние выполнив обратную команду _на каждом хосте_:
```bash
$ curl -s 0x0:4050/debug/read-only-api?set=false
```

## Ссылки
- [wiki: Работа с бекапами](https://wiki.yandex-team.ru/users/hurd/rabotasbekapamibdservisanetworkloadbalancer/)
- [Тикет с описанием учений по восстановлению данных](https://st.yandex-team.ru/CLOUD-83250)
- [Мотивация](https://st.yandex-team.ru/CLOUD-101423)
