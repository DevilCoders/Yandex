[Алерт в Solomon](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts?text=compute+logbroker+topic), [Алерт в Juggler](https://juggler.yandex-team.ru/check_details/?host=yc_compute_logbroker_prod&service=topic-compute-journald)

## Описание
Мониторинги метрик Облачного LogBroker'а. В него складываем логи, затем их вычитывает Yandex LogBroker.

## Схема поставки логов
![](https://jing.yandex-team.ru/files/simonov-d/compute-logs-path.png)

## Проверки
- [topic-journald-messages-read](https://docs.yandex-team.ru/yc-monitoring/compute-api/logbroker/topic-journald-messages-read)
- [topic-journald-messages-written](https://docs.yandex-team.ru/yc-monitoring/compute-api/logbroker/topic-journald-messages-written)
- [topic-journald-partition-max-write-quota-used](https://docs.yandex-team.ru/yc-monitoring/compute-api/logbroker/topic-journald-partition-max-write-quota-used)
- [topic-journald-partition-write-quota-wait](https://docs.yandex-team.ru/yc-monitoring/compute-api/logbroker/topic-journald-partition-write-quota-wait)
- [topic-journald-read-time-lag](https://docs.yandex-team.ru/yc-monitoring/compute-api/logbroker/topic-journald-read-time-lag)
- [topic-journald-time-since-last-read](https://docs.yandex-team.ru/yc-monitoring/compute-api/logbroker/topic-journald-time-since-last-read)
- [topic-journald-topic-write-quota-wait](https://docs.yandex-team.ru/yc-monitoring/compute-api/logbroker/topic-journald-topic-write-quota-wait)

## Дополнительная информация
- https://wiki.yandex-team.ru/cloud/compute/duty/logs/journaldtoyt/
