# lb-rules-inspector

**Значение:** Неконсистентное состояние правил yclb между lb-node с точки зрения конфига шардирования lb-ctrl'а.
**Воздействие:** На разных lb-node настроены разные правила или у них не совпадает состояние.
* `insufficient_replicas` - некоторые правила недостаточно реплицированы по lb-node. Балансировка может работать неправильно (вплоть до дропа трафика), healthcheck может давать false negative результат (в случае, если есть анонс подсети реала).
* `drifted_statuses` - разные статусы у реалов на разных нодах в рамках шарда. Балансировка в этом случае может работать неправильно (вплоть до дропа трафика).
**Что делать:**
1. Подождать. Проверка eventually consistent by design, какая-то из нод могла недавно стартовать и ещё не успела нагнать полное состояние.
2. Посмотреть, какие правила разъехались с каких lb-node: `curl -s 0:4050/debug/inspector` с lb-ctrl, на котором горит проверка.
3. Посмотреть к каким lb-ctrl подключены lb-node с неправильным состоянием: `pssh run 'curl -s 0:4050/debug/watcher | grep node' C@cloud_prod_ylb_stable_lb-ctrl@sas`
4. Далее нужно расселить lb-node с залипшими правилами с этого контроллера:
```
pssh lb-node-sas3.svc.cloud.yandex.net
curl -s 0:4050/debug/connmanager/setup?id=lb-ctrl-rc1b-02.svc.cloud.yandex.net
```
5. Подождать. Во время ожидания нода может быть представлена на двух+ контроллерах - на новом уже есть, но и старый держит состояние на случай быстрого переподключения.
6. Когда алерты озеленятся, а графики выровняются, порестартить залипший lb-ctrl:
```
pssh lb-ctrl-rc1b-03.svc.cloud.yandex.net
sudo systemctl restart yc-loadbalancer-ctrl
```
Репру можно отложить в CLOUD-68068

Отдельно про `drifted_statuses`: есть два способа починить.

1. Способ попроще и поопаснее.
У lb-ctrl есть ручка `curl -s 0:4050/debug/force-update-health`, которая идёт в inspector и для rip'ов с drift'ами выполняет форсированное обновление статусов.
**ОПАСНОСТЕ!**
  1. В списке дрифтов могут быть все правила - если инспектор запустился в момент, когда lb-node рестартовала, и она отдала пустой список правил. В этом случае запускать такой запрос НЕЛЬЗЯ - перегрузим hc-ctrl'ы и ydb.
  2. В списке дрифтов могут быть НЕ ВСЕ правила - если инспектор не запускался на lb-ctrl, который разослал по lb-node неактуальные правила, то ничего не обновится.
  3. Lb-ctrl может так поломать своё состояние, что не поймёт, какие группы он должен обновить.

Можно пробовать этот метод, если на [графике drifted-statuses](https://solomon.cloud.yandex-team.ru/?project=yandexcloud&cluster=cloud_prod_ylb&service=loadbalancer_ctrl&l.name=inspector_drifted_statuses&l.host=!cluster&l.dc=*&graph=auto&b=1d&e=) не наблюдаете 100+ правил.

2. Способ посложнее, но с дополнительными походами в ydb.
  1. Находим лидера (lb-ctrl, который последний опрашивал lb-node). Не забудьте правильное окружение и ДЦ:
  `pssh run "curl -s 0:4050/debug/inspector/status | jq '.is_leader,.last_update_time'" C@cloud_prod_loadbalancer-ctrl_sas`
  Смотрим в true в первой строке.
  2. На контроллере-лидере находим список рипов, которые разъехались:
  ```
  pssh run "curl -s 0:4050/debug/inspector/ | jq '.inspection_result.drifted_statuses[].rule.dst' | sort | uniq" lb-ctrl-rc1b-01.svc.cloud.yandex.net
  ```
  3. На хосте с доступом к ydb (hc-ctrl, lb-ctrl) пишем файлик `drifted.yql`:
  ```
  PRAGMA TablePathPrefix("/global/loadbalancer");

  SELECT hcgi.group_external_id, t.addr, hra.status
  FROM `healthcheck_targets` AS t
  JOIN `healthcheck_healthcheck_results_aggregation` AS hra
    ON hra.target_id = t.id
  JOIN `healthcheck_group_healthchecks__healthcheck_id__group_external_id__index` AS hcgi
    ON hcgi.healthcheck_id = t.healthcheck_id
  WHERE t.addr IN ('fc00::a402:300:a01:4712')
  ```

  NB: в первой строке можно заменить путь с боевой базы на относительно свежий бэкап, подставив число и время в UTC - так базе будет полегче.
  ```
  PRAGMA TablePathPrefix("/global/loadbalancer/backups/healthcheck-hourly/2022-03-03-15-00");
  ```

  NB: правильный `TablePathPrefix` можно грепнуть в конфиге lb-ctrl/hc-ctrl. На примере lb-ctrl:
  ```
  sudo grep -Ew 'database:|endpoint:.*ydb' /etc/yc/loadbalancer-ctrl/config.yaml
  endpoint: ydb-loadbalancer.cloud.yandex.net:2136
  database: /global/loadbalancer
  ```

  NB: в последних строках нужно вписать свои адреса из предыдущего пункта.

  4. Готовим токен:
  ```
export YDB_TOKEN=$(curl -sH "Metadata-Flavor: Google" localhost:6770/computeMetadata/v1/instance/service-accounts/default/token | jq -r ".access_token");
  ```

  5. Выполняем запрос:
  ```
  ydb --endpoint grpcs://ydb-loadbalancer.cloud.yandex.net:2136 --database /global/loadbalancer table query execute --type scan -f drifted.yql
┌────────────────────────────────────────┬─────────────────────────┬────────────────────┐
| hcgi.group_external_id                 | t.addr                  | hra.status         |
├────────────────────────────────────────┼─────────────────────────┼────────────────────┤
| "8583f4d9-bcad-461a-98d8-49c02697700d" | "fc00::2303:300:a02:22" | "OK"               |
├────────────────────────────────────────┼─────────────────────────┼────────────────────┤
| "44764234-c44d-409a-967c-372e5e0d1e47" | "fc00::2303:300:a02:22" | "FAIL_UNSPECIFIED" |
├────────────────────────────────────────┼─────────────────────────┼────────────────────┤
| "87a0fa80-ef42-4339-92d2-dccfc47efc0d" | "fc00::2303:300:a02:22" | "FAIL_UNSPECIFIED" |
└────────────────────────────────────────┴─────────────────────────┴────────────────────┘
  ```

  Endpoint и database грепнули в прошлом пункте. В первой колонке (`group_external_id`) - hc group id, их надо будет накопировать в запрос.

  6. Дёргаем запрос с force update:

  ```
  pssh run "curl -s '0:4050/debug/force-update-health?hc-group=8583f4d9-bcad-461a-98d8-49c02697700d&hc-group=44764234-c44d-409a-967c-372e5e0d1e47'; sleep 30" C@cloud_prod_loadbalancer-ctrl_sas
  ```

  В который подставляем нужные группы.

  7. Ждём. Можно проверить, обновил ли лидер статусы. Также можно поискать события в [jaeger](https://jaeger.private-api.ycp.cloud.yandex.net/search?end=1647338007375000&limit=20&lookback=1h&maxDuration&minDuration&operation=hclb.fono.force_update_health&service=lb-ctrl&start=1647334407375000). Ищем service `lb-ctrl`, operation `hclb.fono.force_update_health`. В трейсах смотрим, ходил ли hc-ctrl в lb-ctrl. Если ходил - статус будет обновлён.
