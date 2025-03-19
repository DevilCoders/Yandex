# lb-ctrl-spreader-refcount

**Значение:** lb-ctrl видит задублированые 5-tuple балансировочных правил. Это может быть временным артефактом из-за аномалий чтения из БД, а может быть следствием нарушения консистентности в CRUD операциях с балансерами.
**Воздействие:** какое-то из правил задублировано, у пользователя могут не работать CRUD операции с балансерами, или останутся лишние правила после Delete операций.
**Что делать:** Посмотреть, какие правила задублировались:
```
# на lb-ctrl
curl -s 127.0.0.1:4050/debug/rules-refcounts
```
Возможно, это дубликат правила в HaaS: https://st.yandex-team.ru/CLOUD-80509#61c9fae509f61f69185b004b . Проверить так:
1. Посмотреть на lb-ctrl rip (адрес из второй колонки)
```
(PROD) elantsev@lb-ctrl-rc1b-01:~$ curl -s 0:4050/debug/rules-refcounts
192.0.2.1:1	[fc00::acc9:200:ac0:8316]:1	tcp	1
```
2. Получить токен от ydb prod https://nda.ya.ru/t/oTfHvto23Vwpci .
3. Дёрнуть запрос с адресом из первого и токеном из предыдущего пункта:
```
export YDB_TOKEN=<token>
curl -H "Authorization: OAuth ${YDB_TOKEN}" "https://kikhouse.svc.kikhouse.bastion.cloud.yandex-team.ru/" -d "SELECT * FROM ydbTable('loadbalancer', '/global/loadbalancer', 'stable/healthcheck/healthcheck_forwarding_rules') where dst_addr = 'fc00::acc9:200:ac0:831e';"
```
Если не похоже (адрес не hcaas'ный), то можно попробовать погреба базу:
Посмотреть в БД, что это за правила (с lb-seed с id правил дубликатов):
```
# lb-seed preprod
ydb --database /pre-prod_global/loadbalancer --endpoint grpc://ydb-loadbalancer.cloud-preprod.yandex.net:2135 table query exec -q 'select * from [stable/healthcheck/healthcheck_forwarding_rules] where id = "00ddcf1c-4f5f-48eb-87e5-4db92e2d2892"'
```
Или, если знаете только группу:
```
# lb-seed preprod
ydb --database /pre-prod_global/loadbalancer --endpoint grpc://ydb-loadbalancer.cloud-preprod.yandex.net:2135 table query exec -q 'select g.group_external_id, fwr.* from [stable/healthcheck/healthcheck_forwarding_group_rules] as g INNER JOIN [stable/healthcheck/healthcheck_forwarding_rules] AS fwr ON g.rule_id = fwr.id WHERE g.group_external_id = "009b85de-09e2-4e8a-9b96-18baadfef4c3"'
```
Спросить у /duty vpc-api, видят ли они у себя такой дубликат (им поможет id группы). Если дубликата у vpc-api нет, но нужно заводить cloudops, в котором удалять из базы правило с более старым updated_at.
