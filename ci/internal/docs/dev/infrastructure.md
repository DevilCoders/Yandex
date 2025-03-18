# Балансеры

## Список

### ci.yandex-team.ru
Балансер поверх ci-backend в [самогоне](https://ui-deploy.n.yandex-team.ru/update/ci_backend/0)

[L7 (awacs)](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/ci.yandex-team.ru/) |
[L3](https://l3.tt.yandex-team.ru/service/13609) |
[Графики](https://yasm.yandex-team.ru/template/panel/balancer_common_panel/fqdn=ci.yandex-team.ru;itype=balancer;ctype=prod;locations=man,sas,vla;prj=ci.yandex-team.ru;signal=default;)


### testenv.yandex-team.ru
Отвечает за доменные имена testenv.yandex-team.ru и ci-old-api.yandex-team.ru (запросы которые ci-backend проксирует в TE)
Развернут поверх Testenv'а в [Deploy](https://deploy.yandex-team.ru/stages/testenv-prod)

[L7 (awacs)](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/testenv.yandex-team.ru/) |
[L3](https://l3.tt.yandex-team.ru/service/13285) |
[Графики testenv.yandex-team.ru](https://yasm.yandex-team.ru/template/panel/balancer_common_panel/fqdn=testenv.yandex-team.ru;itype=balancer;ctype=prod;locations=man,msk,sas;prj=testenv.yandex-team.ru;signal=te_in_yandex-team_ru;) |
[Графики ci-old-api.yandex-team.ru](https://yasm.yandex-team.ru/template/panel/balancer_common_panel/fqdn=testenv.yandex-team.ru;itype=balancer;ctype=prod;locations=man,msk,sas;prj=testenv.yandex-team.ru;signal=ci_in_yandex-team_ru;)

### ci-sp.yandex.net
Балансер над WEB'ами Stream Processor'а. На балансер идет большой трафик (сотни MB/s при обработки посткомитов)
[L7 (awacs)](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/ci-sp.yandex.net/show/) |
[L3](https://l3.tt.yandex-team.ru/service/14366) |
[Графики (awacs)](https://yasm.yandex-team.ru/template/panel/balancer_common_panel/fqdn=ci-sp.yandex.net;itype=balancer;ctype=prod;locations=man,sas,vla;prj=ci-sp.yandex.net;signal=default;) |
[Графики (L3)](https://grafana.yandex-team.ru/d/ByLA2VI7z/l3-vs-ci-sp-yandex-net?) |


### ci-clickhouse.search.yandex.net:8123
Балансер над железными серверами `ci-clickhouse-*`.
Используется в основном [дашборде автосборки](https://datalens.yandex-team.ru/nx85qercjgq0k-autocheck-rt) и дашбордах Прейса. Не используется в ci-backned'е (он ходит напрямую в хосты).


[L7 (awacs)](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/ci-clickhouse/) |
[L3](https://l3.tt.yandex-team.ru/service/7673) |
[Графики](https://yasm.yandex-team.ru/template/panel/balancer_common_panel/fqdn=ci-clickhouse;itype=balancer;ctype=prod;locations=man,sas,vla;prj=ci-clickhouse;signal=default;)

#### Возможные проблемы
* Если балансер отвечат 504, то это не значит, что балансер сломался. Скорее всего, перестали работать запросы в Clickhouse. Нужно попробовать выполнить запрос (который формирует панель в дашборде автосборки) у себя в IDEA/DataGrip.


### ci-observer-api.yandex.net
Балансер над сервисом `ci-observer-api`.
**TODO добавить ссылку на дашборд.**


[L7 (awacs)](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/ci-observer-api.yandex.net/) |
[L3](https://l3.tt.yandex-team.ru/service/14301) |
[Графики (awacs)](https://yasm.yandex-team.ru/template/panel/balancer_common_panel/fqdn=ci-observer-api.yandex.net;itype=balancer;ctype=prod;locations=man,sas,vla;prj=ci-observer-api.yandex.net;signal=ci-observer-api_yandex_net;)
[Графики (L3)](https://grafana.yandex-team.ru/d/xsyMtPV7z/l3-vs-ci-observer-api-yandex-net?)

### ci-observer-api-testing.yandex.net
Балансер над сервисом `ci-observer-api-testing`.
Используется для тестирования


[L7 (awacs)](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/ci-observer-api-testing.yandex.net/) |
[L3](https://l3.tt.yandex-team.ru/service/14300) |
[Графики (awacs)](https://yasm.yandex-team.ru/template/panel/balancer_common_panel/fqdn=ci-observer-api-testing.yandex.net;itype=balancer;ctype=prod;locations=msk,sas;prj=ci-observer-api-testing.yandex.net;signal=ci-observer-api-testing_yandex_net;)
[Графики (L3)](https://grafana.yandex-team.ru/d/xsyMtPV7z/l3-vs-ci-observer-api-testing-yandex-net?)


## Поддержка
[L7 (awacs)](https://wiki.yandex-team.ru/awacs/) (правая панель) | [L7 (support)](https://wiki.yandex-team.ru/l7/) |
[L3 telegram чат](https://t.me/joinchat/ABslOUJ_0OT_31BniPySsA)


## Как завести L3+L7 балансер
[Общая инструкция](https://wiki.yandex-team.ru/awacs/tutorial/l3-l7/)
Пример полученного результата после настройки балансеров: [CI-2569](https://st.yandex-team.ru/CI-2569).
Все примеры и ссылки указаны для проекта ci-observer-api.


### Заводим балансер
1. В нужном проекте деплоя создаем новый балансер. [Пример](https://deploy.yandex-team.ru/balancers/ci-observer-api.yandex.net)
    1.1. **ВАЖНО** Название балансера после создания изменено быть не может.
    1.2. Домен **.yandex-team.ru** для балансеров в которые будут ходить пользователи (люди). Балансеры для сервисов живут в домене **.yandex.net**.
    1.3. Можно создать прямую DNS запись для дебага и тд (*Create DNS record .in.yandex-team.ru...*).
    1.4. *Network macro* `_CINETS_` или `_CITESTNETS_` в зависимости от назначения балансера.
    1.5. *ABC Service id* `CI`

2. Ждем пока создастся балансер и открываем настроки в awacs. [Пример](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/ci-observer-api.yandex.net/show/)
    2.1. *Edit namespace*: Убираем логины из *Owners*, указывавем группу `svc_ci_administration`.
    2.2. *View namespace* -> *Show L3 Balancers*: Создаем новый L3 балансер [по общей инструкции](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/ci-observer-api.yandex.net/l3-balancers/list/ci-observer-api.yandex.net/show/)

3. После создания L3 балансера, по общей инструкции:
    3.1. Настроить туннели через UI awacs
    3.2. Добавить записи в DNS (параметры `type AAAA`, `ttl 300`)
    3.3. Выдать доступы к сервису в Puncher (в том числе для группы `svc_ci`)

4. Добавляем балансер со всеми ссылками в [infrastructure.md](https://a.yandex-team.ru/arc/trunk/arcadia/ci/docs/dev/infrastructure.md)


### Настройка алертинга балансера
1. Устанавливаем аваксовский стандартный алертинг [в никуда](https://nanny.yandex-team.ru/ui/#/awacs/namespaces/list/ci-observer-api.yandex.net/alerting/)
2. Выводим джагглеровские алерты на [дашборд CI](https://juggler.yandex-team.ru/dashboards/ci) в соответствующую секцию (фильтрация по неймспейсу)
3. Для продового балансера настраиваем [нотификации на алерты](https://juggler.yandex-team.ru/notification_rules/?query=namespace%3Ddevtools.ci) в *slack*, *telegram*, *phone_escalation* (также по неймспейсу)

# Базы данных

## SP/CI-backend

### Clickhouse
https://yav.yandex-team.ru/secret/sec-01e5jtansqjwta8y9z72g65cy3/explore/versions

#### На железе
* ci-clickhouse-01.search.yandex.net:8123 (Выведен из эксплуатации из-за потери финки, но пока не зачищен)
* ci-clickhouse-02.search.yandex.net:8123
* ci-clickhouse-04-sas.search.yandex.net:8123
* ci-clickhouse-05.search.yandex.net:8123

`jdbc:clickhouse://ci-clickhouse-05.search.yandex.net:8123`
`<clickhouse.user>/<clickhouse.password>`

#### Облачный runs
https://yc.yandex-team.ru/folders/fooo2f1maha60gvok8ml/managed-clickhouse/cluster/mdb5hfu4l1f0job8l8nt \

#### Облачный all-runs
https://yc.yandex-team.ru/folders/fooo2f1maha60gvok8ml/managed-clickhouse/cluster/mdbckcguevu91iu04akc \


### MongoDB

https://yav.yandex-team.ru/secret/sec-01e5jtansqjwta8y9z72g65cy3/explore/versions

#### На железе
`mongodb://ci-clickhouse-02.search.yandex.net:27037,ci-clickhouse-03.search.yandex.net:27037/testenv?replicaSet=testenv_sp&readPreference=primary&connectTimeoutMS=30000&socketTimeoutMS=80000&w=majority&readConcernLevel=majority` \
`<mongodb.user>/<mongodb.password>`

#### Облачный
```
#!/bin/bash

mongo --norc \
        --tls \
        --tlsCAFile /usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt \
        --host 'rs01/sas-siqxq83n6frm4fqa.db.yandex.net:27018,vla-4e816rnyzblgzs6x.db.yandex.net:27018' \
        --ipv6 \
        -u <mongodb.runs.user> \
        -p <mongodb.runs.password> \
        testenv
```

P.S. Такой же, как и в runs. Пароль, например, можно читать из файла: `cat ~/.ci/mongo-prod-stable.txt`

#### Облачный sharded-runs
```
#!/bin/bash

mongo --norc \
        --tls \
        --tlsCAFile /usr/local/share/ca-certificates/Yandex/YandexInternalRootCA.crt \
        --host 'sas-s44uu46vusznmkay.db.yandex.net,vla-vc06xkqcjmwe0m4m.db.yandex.net,' \
        --ipv6 \
        -u <mongodb.runs.user> \
        -p <mongodb.runs.password> \
        sp-sharded \

```

### MySql

[Ссылка в YC](https://yc.yandex-team.ru/folders/fooo2f1maha60gvok8ml/managed-mysql)


### Хранилище в S3
[Основная документация S3](https://wiki.yandex-team.ru/mds/s3-api/authorization/)

Что нужно сделать:
1) [Проверить наличие роли](https://wiki.yandex-team.ru/mds/s3-api/authorization/#s3-roles)
2) [Получить токен по указанной ссылке](https://wiki.yandex-team.ru/mds/s3-api/authorization/#upravlenieaccesskeys) (**Важно:** для тестинг/продакшен процессов токен надо получать от имени соответствующего робота)
3) [Получить ключ доступа](https://wiki.yandex-team.ru/mds/s3-api/authorization/#sozdanieaccesskey)
CI - приложение 7838. В данном примере получаем роль "admin"
	* В проде: `curl -XPOST -H"Authorization: OAuth <токен из п1>" "https://s3-idm.mds.yandex.net/credentials/create-access-key" --data "service_id=7838" --data "role=admin"`
	* С тестингом S3 наши процессы не работают, доступ туда не нужен.
	Ключи кладутся в `~/.aws/credentials` в стандартном формате:
	```
	[default]
	aws_access_key_id=...
	aws_secret_access_key=...
	```
4) Созданные бакеты можно увидеть в проекте "ci" [в консоли YC](https://yc.yandex-team.ru/folders/foonmi1hkk792qrrgsi6/storage/buckets):
	* "ci-bazinga"
	* "ci-bazinga-testing"
	* "ci-storage-bazinga"
	* "ci-storage-bazinga-testing"
5) Делаем запросы
	* `aws --endpoint-url=http://s3.mds.yandex.net s3 ls s3://ci-bazinga-testing`


### Список всех используемых секретов
* [https://yav.yandex-team.ru/?tags=ci](https://yav.yandex-team.ru/?tags=ci)
* Секреты Testenv, CI Backend и SP имеют дополнительный тег `testenv`

Все секреты на Prod и Prestable доступны для чтения и редактирования людям из группы "Администрирование" в проектах CI и Testenv для секретов CI и Testenv/CIB/SP соответственно.
