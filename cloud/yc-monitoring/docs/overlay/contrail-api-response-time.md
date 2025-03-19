[Алерт contrail-api-response-time в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcontrail-api-response-time)

## Что проверяет

- время ответа contrail-api на https порту (8482) по логам nginx

- [исходный код алерта](https://bb.yandex-team.ru/projects/CLOUD/repos/vpc-solomon/browse/alerts/oct/contrail_api_response_time.j2)

## Если загорелось

- изучить nginx-логи в `/var/log/contrail/contrail-api.log` и `/var/log/nginx/contrail-api_ssl_access.log`