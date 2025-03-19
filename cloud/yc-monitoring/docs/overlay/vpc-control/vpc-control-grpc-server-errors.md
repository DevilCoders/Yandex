[Алерт vpc-control-grpc-server-errors в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dvpc-control-grpc-server-errors)

## Что проверяет

Ошибки входящих grpc запросов. Горит **красным**, если за последние десять минут были ошибки.

## Если загорелось

- посмотреть метрику grpc_server_response_total для vpc-control

- посмотреть какие еще алерты загорелись по vpc-node/vpc-control

- смотреть логи сервиса `yc-vpc-control`, чтобы понять, с чем конкретно связаны ошибки
