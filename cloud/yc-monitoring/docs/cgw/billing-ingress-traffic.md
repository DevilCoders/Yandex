# billing-ingress-traffic

**Значение**: за последний час есть невалидные метрики, которые балансеры отправили в биллинг.
**Воздействие**: не биллим/некорректно биллим пользователей
**Что делать**:
1. Посмотреть ошибки джоб (могут падать при наличии проблем у YT)
2. Посмотреть таблички в YT по ссылкам ниже
3. Разобраться в причинах невалидности метрик
**Данные**:
* Таблички в YT: [billing-nlb-traffic prod](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud/billing/invalid-metrics/prod/yc%23billing-nlb-traffic-0), [billing-nlb-traffic preprod](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud/billing/invalid-metrics/preprod/yc%23preprod%23billing-nlb-traffic-0)
* Джобы в Sandbox: [прод](https://sandbox.yandex-team.ru/scheduler/24390/view), [препрод](https://sandbox.yandex-team.ru/scheduler/24392/view) (запускаются раз в 5 минут)
Подробнее можно почитать тут:
[https://clubs.at.yandex-team.ru/ycp/2986](https://clubs.at.yandex-team.ru/ycp/2986)
[https://wiki.yandex-team.ru/cloud/devel/billing/invalidmetricsmonitoring/](https://wiki.yandex-team.ru/cloud/devel/billing/invalidmetricsmonitoring/)
[https://clubs.at.yandex-team.ru/ycp/2147](https://clubs.at.yandex-team.ru/ycp/2147)
