[Алерт contrail-control-stuck в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-control-stuck)

**Внимание, проверка давно не стреляла по изначальной проблеме!**

**Сейчас может загораться из-за того, что contrail-control не ответил на интроспекцию.**

В этом случае смотри мониторинг `contrail-control` выше.

## Что проверяет

«Залипание» сервиса `contrail-control` с помощью [специально запроса](https://bb.yandex-team.ru/projects/CLOUD/repos/salt-formula/browse/opencontrail/control/mon/bin/contrail-control-stuck#8) к интроспекции.

## Если загорелось

Бить тревогу и что-то пытаться чинить стоит только в случае, если мониторинг горит долго или есть явный импакт.

Для починки смотри [инструкцию](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/contrail-control-got-stuck/).