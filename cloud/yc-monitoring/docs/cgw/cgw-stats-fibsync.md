# cgw-stats-fibsync
[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?project=&query=service%3Dcgw-stats-fibsync)

## Проверяет
Проверяет соответствие маршрутной информации из ```gobgp``` в ```vpp```. Реагирует на отсутствие маршрутов и/или наличие "лишних" маршрутов.

## Что делать
Может давать false-positive результат (см. CLOUD-96818), как правило повторный вызов
```
jurun -r cgw-stats
```
озеленяет проверку.
Если мониторинг горит несколько циклов для одних и тех же префиксов, нужно смотреть:

* Проверить, что этот ip есть или нет в ```vpp``` - ```sh ip{6} fib <prefix>```
* Проверить, что этот ip есть в ```gobgp``` - ```gobgp global rib -a <family> ```

В случае несоответствия состояния - прикопать в дежурный тикет всю информацию выше. Рекавери - ```sudo yavpp-restart```
