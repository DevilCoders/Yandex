# ydb_timings

**Значение:** Возросли тайминги запросов к YDB в каком-то из go-сервисов
**Воздействие:** Деградация производительности, частичная или полная неработоспособность сервиса
**Что делать:** Solomon alert. Увидеть проблемы можно на графиках типа
https://solomon.cloud.yandex-team.ru/?cluster=cloud_prod_ylb&project=yandexcloud&service=loadbalancer_ctrl&graph=yandexcloud-ylb-prod-app-ydb-per-request-timings&env=prod&dc=*&name=ydb_*&host=lb-ctrl-*&b=2019-12-22T15%3A57%3A04.294Z&e=2019-12-23T03%3A57%3A04.294Z
Выбираем нужный график по связке окружение + сервис. График должен находится на борде сервиса.
По графику должно быть понятно, какой запрос начал притормаживать.
* Если возросли все тайминги разом - скорее всего роблема на стороне YDB, /duty ydb
* В случае если вырос один / несоколько запросов нужно призывать ответсвенного за сервис /duty lb
* Возможным вариантом решения будет остановка по одному контроллеру в зоне:
`for zone in a b c ; do pssh hc-ctrl-rc1${zone}.svc.cloud-preprod.yandex.net sudo systemctl stop yc-healthcheck-ctrl ; done`
`for zone in a b c ; do pssh lb-ctrl-rc1${zone}.svc.cloud-preprod.yandex.net sudo systemctl stop yc-loadbalancer-ctrl ; done`
Это уберёт нагрузку с базы, возможно, её отпустит. После этого остановленные хосты вводить по одному, с большими интервалами.
