# hc-node-update-target-status

**Значение:** hc-node получают ошибки при отправке результатов healthcheck'ов в hc-ctrl в ручку UpdateTargetStatus.
**Воздействиие:** target'ы не могут поменять здоровье, здоровые таргеты не вводятся в балансировку, больные не выводятся.
**Что делать:**
1. Посмотреть графики [hc-node UpdateTargetStatus_fail](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_ylb&service=healthcheck_node&l.host=cluster&l.name=client_grpc__healthcheck_v1alpha_HealthcheckInternalService__UpdateTargetStatus_fail&graph=auto&transform=differentiate), на них видна причина. Если там даминирует DeadlineExceeded - это либо сеть, либо затуп в hc-node/hc-ctrl, NotFound - hc-ctrl не знает такого target'а, Unavailable - ошибка внутри hc-ctrl.
2. Посмотреть графики [hc-ctrl UpdateTargetStatus_fail](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_ylb&service=healthcheck_ctrl&l.name=grpc__yandex_cloud_internal_healthcheck_v1alpha_HealthcheckInternalService__UpdateTargetStatus_fail&l.host=cluster&graph=auto&transform=differentiate). Если совпадают с предыдущим графиком, значит, дело не в сети. Нужно попробовать локализовать проблему до конкретного hc-ctrl.
3. Снять состояния таргетов с hc-node и hc-ctrl, ydb с hc-ctrl:
* `pssh run -p9 'curl -s 0x0:4050/debug/targets > targets.txt' C@cloud_prod_ylb_stable_hc-node`
*  `pssh run -p9 'curl -s 0x0:4050/debug/targets > targets.txt' C@cloud_prod_ylb_stable_hc-ctrl`
* `pssh run -p9 'curl -s 0x0:4050/debug/ydb/status > ydb.txt' C@cloud_prod_ylb_stable_hc-ctrl`
4. Снять состояние горутин с hc-node и hc-ctrl:
* `pssh run 'curl -s "0x0:4050/debug/pprof/goroutine?debug=2" > gorotines.txt' C@cloud_prod_ylb_stable_hc-node`
* `pssh run 'curl -s "0x0:4050/debug/pprof/goroutine?debug=2" > gorotines.txt' C@cloud_prod_ylb_stable_hc-ctrl`
5. Рестартнуть проблемный hc-ctrl (если это не последний живой hc-ctrl в зоне): `sudo systemctl restart yc-healthcheck-ctrl`.
