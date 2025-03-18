# ElasticSearch

1. [Deploy](#deploy)
1. [Monitoring](#monitoring)
1. [Elastic API](#elastic-api)
1. [Kibana](#kibana)
1. [Curator](#curator)
1. [Filebeat](#filebeat)


## Deploy
[Stage c ElasticSearсh](https://deploy.yandex-team.ru/project/antiadb-elasticsearch)

В antiadb-elasticsearch 3 deploy-unit:
* Data Nodes (в них Эластик хранит логи)
* Master Nodes (назначают задания другим нодам, не хранят информацию),
* Search Nodes (поисковые ноды, делают запросы в Кластер, на них также поднимается Кибана)

Интерфейс Кибаны сейчас доступен по https://ubergrep.in.yandex-team.ru.

Подробнее о том, где настроено это DNS имя: https://deploy.yandex-team.ru/balancers/ubergrep.yandex.net.

Также Интерфейс Кибаны можно найти по url, соответствующему адресу машинки,
на которой она поднялась.

Для того чтобы сделать изменения в конфигурации Эластика:
* Собираем docker из папок elastic/elasticsearch и elastic/kibana
и пушим их в registry.yandex.net
Для этого надо выполнить в папках elasticsearch и  kibana соответственно:
```
ya package package.json --docker --docker-repository=antiadb --target-platform default-linux-x86_64 --docker-push
```
* Обновляем версии контейнера и команды запуска в dctl/spec.yml и dctl/settings.json,
(обновлять конфигурацию можно также в Deploy.Config,
однако сейчас в интерфейсе недоступна полная функциональность,
например нельзя выделить persistent volume)
* Запускаем скрипт из папки dctl (скрипт заменяет template-параметры в файле template-spec.yml,
на соответствующие настройки в settings.json и заливает полученный spec в Deploy).
Ожидается, что скипт выведет "Put stage: antiadb-elasticsearch"
Выполнить в папке dctl:
```
ya make
./dctl
```

Конфигурацию и Индексы Эластик хранит в папке /perm.

Логи Эластика можно посмотреть на машинке в папке /porto_log_files.

**ВНИМАНИЕ**

* Если Вы меняете название deploy stage или deploy unit, то соотвествующие
изменения надо сделать в файлах [launch_*.sh](https://arcanum.yandex-team.ru/arc/trunk/arcadia/antiadblock/elastic/elasticsearch/launch),
так как в них резолвятся hostnames
машинок с эластиком (скрипт deploy_unit_resolver ходит в Deploy и по названию
stage и unit получает информацию об endpoints).
Deploy_unit_resolver используется также в antiablock/cryprox/log_shippers для [Filebeat](#filebeat).
Тагже названия endpoints используются Solomon агентом.


## Monitoring
[Дашборд в Соломон](https://solomon.yandex-team.ru/?project=Antiadblock&dashboard=ElasticSearch)

Чтобы собирать метрики Solomon ходит в Elastic по [API](https://www.elastic.co/guide/en/elasticsearch/reference/current/cat-nodes.html).
В Solomon эти метрики называются [elastic_stats](https://solomon.yandex-team.ru/?project=Antiadblock&service=elastic_stats).

В [Juggler](https://juggler.yandex-team.ru/notification_rules/?query=rule_id=5eea3f442c7c44006f2cdd1f) настроены alerts на все основные метрики.

Важным показателем "здоровья" кластера является процент assigned shards, если он опустился ниже 85%, то скорее всего
что-то пошло не так.

Если Elastic перешел в состояние red (то есть все плохо),
то он может больше не отвечать на запросы по API и метрики не будут появляться.
Самый простой способ вывести кластер из состояния red, удалить соответствующие [индексы](https://ubergrep.in.yandex-team.ru/app/kibana#/management/elasticsearch/index_management/indices?_g=()).
Про удаление индексов и ротацию в разделе [Curator](#curator).

## Elastic API
Как понять, что кластер поднялся:

```http request
GET http://hostname:8890/_cluster/health
```

hostname - любая из машинок с эластиком в Deploy

Ожидаемый ответ (проверяем):
```json
{
  "status": "green",
  "number_of_nodes": 36,
  "number_of_data_nodes": 30,
  "unassigned_shards": 0,
  "delayed_unassigned_shards": 0
}
```
Если  unassigned_shards != 0, чтобы узнать что не так и получить подробную информацию о причине:

```http request
GET http://hostname:8890/_cluster/allocation/explain?pretty
```

Чтобы вывести построчно незаасайненые шарды выполняем:
```
curl -s -XGET localhost:8890/_cat/shards?h=index,shard,prirep,state,unassigned.reason| grep UNASSIGNED
```

Если шарды безнадёжно утеряны и они вам не нужны и стоит цель востановить стейт кластера, то можно дропнуть заафекченные индексы:
```
curl -XDELETE 'localhost:8890/<index_name>/'
```

ниже пример команды удаляющей все стейджинг индексы:
```
curl -s -XGET localhost:8890/_cat/shards?h=index,shard,prirep,state,unassigned.reason | grep staging | awk '{print $1}' | xargs -I % curl -XDELETE localhost:8890/%/
```

Подробнее про API: https://www.elastic.co/guide/en/elasticsearch/reference/current/docs.html

## Kibana
Кинаба хранит свои настройки в види индексов в Elastic.
На случай если что-то упало и эти индексы были потеряны,
можно восстановить из файла kibana/interface_settings.json.
Для этого нужно зайти в Kibana - Managment - Saved Objects
найти Import и выбрать файл kibana/interface_settings.ndjson.
Для каждой визуализации нужно выбрать соотвествующие ей индекс-паттерны.

Чтобы изменить количество реплик индексов Кибаны:

```http request
# apply to system indices: .kibana_1, .kibana_2, .tasks, .kibana_task_manager_1, .apm-agent-configuration

PUT http://c6rtcapi42mr2255.sas.yp-c.yandex.net:8890/.kibana_1/_settings
Content-Type: application/json

{
    "index" : {
        "auto_expand_replicas": false,
        "number_of_replicas" : 2
    }
}
```

**ВНИМАНИЕ**

Попытка удалить (через интерфейс или API поисковых нод) индексы, в которые СЕЙЧАС пишет filebeat, может приводить к "неопределенному поведению".

В частности, возможно отделение master nodes от кластера,
об этом может сигнализировать "master_not_discovered_exception", при запросах в [API](#elastic-api),
или падение RAM на графике [RamUsagePercentMasterNodes](https://solomon.yandex-team.ru/?project=Antiadblock&cluster=elastic-master-nodes&service=elastic_stats&l.stats=ramPercent&graph=auto)

Если вы получили master_not_discovered_exception, то можно начать с [передеплоя](#deploy) кластера.

#### Если не появляются новые данные

Если закончилось место, то нода может перейти в режим `read-only` (95% used space),
то вывести ее из read only можно следующим образом:

Уменьшаем на 1 unit_count в indicies-rotator-actionfile.yml
(curator/indicies-rotator-actionfile.yml на машинке)
Перезапускаем curator: /launch_curator.sh

После этого надо убрать режим `read-only`:

1. Заходим в нужную кибану:
2. Идем в `Dev Tools` и выполняем `GET _settings`
3. Видим там `"read_only_allow_delete": "true"`, значит кластер в `read-only`
4. Вбиваем
```
  PUT _settings
    {
    "index": {
    "blocks": {
    "read_only_allow_delete": "false"
    }
    }
    }
```

Все должно стать хорошо :)

Ccылка на документацию куратора: https://www.elastic.co/guide/en/elasticsearch/client/curator/current/index.html.
Installation guide for CentOS: https://www.elastic.co/guide/en/elasticsearch/client/curator/5.8/yum-repository.html.

## Filebeat
Filebeat собирает логи Cryprox и записывает их в Elastic.
Настройки filebeat можно посмотреть в arcadia/antiadblock/cryprox/log_shippers.
В настройках filebeat указываются количество реплик и шардов для индексов Эластика.
Текущее количество реплик - 1, количество шардов - 10.

На процесс filebeat в Nanny стоит лимит CPU в 1 ядро, так как начало заливки логов
может приводить к "взрыву" CPU на машинках Cryprox и ElasticSearch, который должен пойти на спад в течении 1-2 часов.
При запуске процесса Filebeat пытается записать в Elastic все логи, которых там еще нет (по мнению filebeat).
То есть если filebeat был выключен долго, накапливается много незаписанных логов, и filebeat пытается их все залить в Elastic.

#### Управление заливкой  логов

Так как в начале заливки логов потребляется много ресурсов, то может быть удобно (и безопасно) заливать логи разных ДЦ по очереди.
Включаются/выключаются filebeat в разных ДЦ с помощью [скрипта](https://arcanum.yandex-team.ru/arc/trunk/arcadia/antiadblock/cryprox/log_shippers/filebeat/update_filebeat/__main__.py)
и sandbox resourse (ANTIADBLOCK_LAUNCH_FILEBEAT).
Ресурс ANTIADBLOCK_LAUNCH_FILEBEAT - файл типа [if_launch.json](https://arcanum.yandex-team.ru/arc/trunk/arcadia/antiadblock/cryprox/log_shippers/filebeat/update_filebeat/if_launch.json).
Скрипт раз в минуту скачивает последний загруженный файл типа ANTIADBLOCK_LAUNCH_FILEBEAT из sandbox,
и в зависимости от него запускает (или не запускает) filebeat.

1 напротив ДЦ в файле  соответсвует включенному трафику из этого ДЦ, 0 наоборот.

По умолчанию (если файл не загружен) все ДЦ включены.

Чтобы загрузить файл в sandbox:

```
ya upload --type=ANTIADBLOCK_LAUNCH_FILEBEAT [filename]
```

По умолчанию sandbox хранит файлы 14 дней, чтобы загрузить файл навсегда: --ttl=inf.
