[Алерт contrail-api-errors в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-api-errors)

## Что проверяет

- наличие ответов от contrail-api с кодом 5xx в логе contrail-api

*(примечание) не загорается при рестартах contrail-api*

- [исходный код алерта](https://bb.yandex-team.ru/projects/CLOUD/repos/vpc-solomon/browse/alerts/oct/contrail_api_errors.j2)

## Если загорелось

- изучить логи в `/var/log/contrail/contrail-api.log`