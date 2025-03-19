# cgw-stats-own-announces
[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?project=&query=service%3Dcgw-stats-own-announces)

## Проверяет
Проверяет, что анонсы из конфига выставлены.

## Что делать
Может валидно гореть красным на старте. В случае, когда все анонс-группы подняты (проверить так):
```
curl 127.0.0.1:50099/debug/state/ag
```
а проверка красная - прикопай в дежурный тикет. На эту проверку настроен авторекавери.
