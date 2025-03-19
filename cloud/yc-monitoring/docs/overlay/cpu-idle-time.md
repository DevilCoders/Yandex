[Алерт cpu-idle-time в джаглере](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcpu-idle-time)

## Что проверяет

Объем свободных ресурсов (CPU) на SVM.

## Если загорелось

- выяснить, кто тратит CPU:

  - посмотреть на [основной дэшборд](https://solomon.yandex-team.ru/?project=yandexcloud&dashboard=oct_main_prod)

  - зайти на инстанс и изучить htop

- если другого импакта нет, рестартовать какие-либо сервисы **не рекомендуется**

- если часто загорается, завести тикет