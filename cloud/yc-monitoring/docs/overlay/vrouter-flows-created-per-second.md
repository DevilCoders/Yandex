[Алерт vrouter-flows-created-per-second в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dvrouter-flows-created-per-second)

## Что проверяет

Скорость создания новых флоу в вроутере. Горит **красным**, если за последние пять минут среднее число запросов на новые флоу **превысило 15 000 в секунду**. **Желтым**, если **превысило 10 000 в секунду**. 

## Если загорелось

- возможно, идёт DDoS. Ознакомься с [инструкцией](https://wiki.yandex-team.ru/cloud/devel/sdn/duty/ddos-detection/).

- в целом, максимально допустимое значение для вроутера — 20 000 новых флоу в секунду.

Этот мониторинг построен на основе [алерта из соломона](https://solomon.yandex-team.ru/admin/projects/yandexcloud/alerts/vrouter_flows_created_per_second_prod).