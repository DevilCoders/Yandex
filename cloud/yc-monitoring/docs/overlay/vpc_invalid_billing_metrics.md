[Алерт vpc_invalid_billing_metrics в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dvpc_invalid_billing_metrics)

Состоит из двух проверок `billing-traffic` и `billing-fip`

## Что проверяет

Проверяет не было ли за последний час невалидных метрик, которые мы отправили в биллинг.

Данные берутся из табличек в YT: [billing-traffic prod](https:*yt.yandex-team.ru/hahn/navigation?path=*home/cloud/billing/invalid-metrics/prod), [billing-traffic preprod](https:*yt.yandex-team.ru/hahn/navigation?path=*home/cloud/billing/invalid-metrics/preprod/yc-pre%23billing-sdn-traffic-0), [billing-fip prod](https:*yt.yandex-team.ru/hahn/navigation?path=*home/cloud/billing/invalid-metrics/prod/yc%23billing-sdn-fip-0), [billing-fip preprod](https:*yt.yandex-team.ru/hahn/navigation?path=*home/cloud/billing/invalid-metrics/preprod/yc-pre%23billing-sdn-fip-0). Собираются с помощью регулярно (раз в пять минут) запускаемых джоб [в Sandbox](https://sandbox.yandex-team.ru/schedulers?author=kostya-k&task_type=ASSERT_NO_NEW_ROWS&limit=20)

## Если загорелось

Посмотреть таблички в YT по ссылкам выше, разобраться в причинах невалидности метрик. Подробнее можно почитать тут:

[Этушка, пост №1](https://clubs.at.yandex-team.ru/ycp/2986)

[Этушка, пост №2](https://clubs.at.yandex-team.ru/ycp/2147)

[Вики биллинга](https://wiki.yandex-team.ru/cloud/devel/billing/invalidmetricsmonitoring/)

