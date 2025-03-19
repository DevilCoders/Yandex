[Алерт в Juggler](https://juggler.yandex-team.ru/aggregate_checks/?query=service%3Dcompute-underlay-ipv6)

## compute-underlay-ipv6
Проверяет, что инстансы с Underlay не используют чужие IPv6 Underlay адреса.

Такое может происходить, если в гостевой ОС адрес на интерфейсе не обновляется через SLAAC при каждой загрузке,
а прописан статически в конфиге.

## Подробности

Сетевая карта `eth0` нарезается на несколько VF, каждый VF имеет свой MAC адрес.

Когда мы отдаем какую-то VF гостю, мы из MAC-адреса и IPv6-префикса, полученного от TOR-а, вычисляем по SLAAC IPv6-адрес,
и его считаем выделенным инстансу (сохраняем в `state.json` и в базу).

Проверка проходит по всем MAC-адресам VF-ок, вычисляет для каждой IPv6, и делает ping на него. После ping-а смотрит
в neighbor cache - доступен ли адрес, и если да, на каком MAC. Если доступен на неожиданном MAC - загорается CRIT.

## Диагностика

Допустим, горит алерт:
```
4 reachable addresses. 1 problem(s): address '2a02:6b8:bf00:1042:9cbe:4ff:fea7:2081' of instance 'c8r24ejk9021uqaqsac5' is reachable on wrong mac '9e:be:08:bf:8c:df' (expected '9e:be:04:a7:20:81')
```

Расшифруем:
- `2a02:6b8:bf00:1042:9cbe:4ff:fea7:2081` - проблемный Underlay IPv6 адрес (его кто-то занял)
- `c8r24ejk9021uqaqsac5`                  - законный владелец адреса (может быть `None`, если адрес не выделен)
- `9e:be:08:bf:8c:df`                     - MAC-адрес нарушителя (позволяет его однозначно идентифицировать)

Что делаем:
- находим хост (compute-node) нарушителя:
```
pssh run -p15 "sudo ip link | grep 9e:be:08:bf:8c:df || true" C@cloud_preprod_compute 2>/dev/null | grep MAC -B2
```
```
sas09-ct33-30.cloud.yandex.net:
OUT[0]:
    vf 8 MAC 9e:be:08:bf:8c:df, spoof checking off, link-state auto, trust on, query_rss off, query_rss off
```
- ищем инстанс на хосте:
```
pssh run "sudo find /var/lib/yc/compute-node/instances/ -name state.json | xargs grep -l \$(cat /var/run/vf-interfaces.json | jq -r '.[] | select(.mac_address == \"9e:be:08:bf:8c:df\") | .id')" sas09-ct33-30.cloud.yandex.net
```
```
sas09-ct33-30.cloud.yandex.net:
OUT[0]:
/var/lib/yc/compute-node/instances/c8rl76d0c8hohn7vbfkp/state.json
```
- получаем информацию по инстансу:
```
ycp --profile <PROFILE> compute instance get c8rl76d0c8hohn7vbfkp | head -n10
```
```
id: c8rl76d0c8hohn7vbfkp
folder_id: aoe59dqr8mabq1af8539
created_at: "2022-02-04T09:31:38Z"
name: dns-infra-2-preprod-sas1
labels:
  abc_svc: ycdns
  env: pre-prod
  layer: iaas
zone_id: ru-central1-b
platform_id: standard-v2
```
- идем к владельцу инстанса либо дежурному, и просим:
  - (сейчас) исправить адрес на инстансе либо остановить инстанс,
  - (в будущем) исправить схему получения адреса на SLAAC (см. инструкцию ниже).

## Ссылки

- [Инструкция по настройке Underlay v6 на инстансе](https://wiki.yandex-team.ru/cloud/devel/sdn/svm-network-configuration/#ipv6vanderlee) - сюда отправляем владельца SVM, которая занимала чужой адрес
- Пример проблемы: [COMPUTESUPPORT-125](https://st.yandex-team.ru/COMPUTESUPPORT-125)
- Улучшение схемы работы с Underlay: [CLOUD-75171](https://st.yandex-team.ru/CLOUD-75171)
