# vpp-cpu-load

**Значение:** повышенная нагрузка на lb-nod
**Воздействие:** можем дропать пакеты в этот момент, так как vpp не успевает обрабатывать
**Что делать:** см. [алерт в соломоне](https://solomon.cloud.yandex-team.ru/admin/projects/yandexcloud/alerts/yandexcloud-prod_ylb_vpp_cpu_load), сделано по рекомендациям из документации к vpp.
смотреть есть ли такая проблема на соседних нодах в регионе, если нет, попробовать рестартануть vpp `systemctl restart vpp`
Найти наиболее загруженную ноду можно по `pssh run -p10 'sudo vppctl sh runtime | grep average' C@cloud_prod_ylb_stable_lb-node`, смотрим у кого самый высокий `average vectors/node`.
