======================================
Создание новой clickhouse базы в dbaas
======================================

Документация про dbaas
----------------------
Документация находится по следюущим ссылкам:
  * https://wiki.yandex-team.ru/dbaas/quickstart/#clickhouse - quick start для создания новой базы
  * https://api.db.yandex-team.ru/ - swagger-документация по вызовам api
  * https://wiki.yandex-team.ru/dbaas/api/ - описание api

Создание новой базы
-------------------

Получение прав доступа с разработческих машин к api
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

По умолчанию с разработческих машин нет доступа к api. Делается с помощью запроса https://golem.yandex-team.ru/fwreqhole.sbml (в качестве приемника указать api.db.yandex-team.ru, в качестве источника разработческие машины). После того как сделано, доступ проверятся **telnet api.db.yandex-team.ru 443** .

Получение прав для доступа к созданным базам
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Делается с помощью запроса https://golem.yandex-team.ru/fwreqhole.sbml (в качестве приемника указать  _PGAASINTERNALNETS_ , порты 8123 и 9000, в качестве источника разработческие машинки).

Получение ключа для авторизации
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Все запросы должны быть авторизованы OAuth-ключем . Документация по получению ключа здесь: https://wiki.yandex-team.ru/dbaas/api/#autentifikacijaiavtorizacija .

Запрос на создание базы
~~~~~~~~~~~~~~~~~~~~~~~

Конфиг для запроса выглядит следующим образом (пусть будет файл **1.data**)

::
  {
    "cluster_type": "clickhouse_cluster",
    "name": "gencfg_test1",
    "environment": "qa",
    "options": {
        "database": {
            "name": "gencfg_test1"
        },
        "flavor": "db1.nano",
        "version": "latest",
        "volume_size": 10737418240,
        "nodes": {
            "man": 1,
            "vla": 1
        },
        "users": [
            {"name": "kimkim", "password": "aevohngaC0fahchaeh3yohp7eic"}
        ],
        "clickhouse": {
        }
     }
  }

Сам запрос выглядит так:

::
  kimkim@minos:~/work/gencfg.svn(:trunk)$ curl "https://api.db.yandex-team.ru/api/v1.0/project/0/cluster" --data-binary "@1.data" --header "Content-type: application/json" --header "Authorization: OAuth MY_OAUTH_TOKEN"
  {
    "task_id": "6b3d9337-2453-46d9-b3f4-adde26f26417"
  }
  kimkim@minos:~/work/gencfg.svn(:trunk)$

После задания запроса мы получили **task_id**, в котором выполняется запрос на создание базы. Нужно в цикле проверять, пока этот запрос не будет выполнен:

::
  kimkim@minos:~/work/gencfg.svn(:trunk)$ curl "https://api.db.yandex-team.ru/api/v1.0/project/0/task/6b3d9337-2453-46d9-b3f4-adde26f26417" --header "Authorization: OAuth MY_OAUTH_TOKEN"
  {
    "cid": "cbef5b21-16fa-4c6a-9b2f-f00b9ccc4b22", 
    "created": "2018-02-19T11:33:05.737780+00:00", 
    "started": "2018-02-19T11:33:06.674931+00:00", 
    "status": "in progress", 
    "task_id": "6b3d9337-2453-46d9-b3f4-adde26f26417"
  }
  kimkim@minos:~/work/gencfg.svn(:trunk)$ curl "https://api.db.yandex-team.ru/api/v1.0/project/0/task/6b3d9337-2453-46d9-b3f4-adde26f26417" --header "Authorization: OAuth MY_OAUTH_TOKEN"
  {
    "cid": "cbef5b21-16fa-4c6a-9b2f-f00b9ccc4b22", 
    "created": "2018-02-19T11:33:05.737780+00:00", 
    "finished": "2018-02-19T11:36:24.556286+00:00", 
    "started": "2018-02-19T11:33:06.674931+00:00", 
    "status": "finished", 
    "task_id": "6b3d9337-2453-46d9-b3f4-adde26f26417"
  }
  kimkim@minos:~/work/gencfg.svn(:trunk)$

Этот запрос вернет ид кластера **cid** , по которому дальше можно получить информацию о созданной базе

::
  kimkim@minos:~/work/gencfg.svn(:trunk)$ curl "https://api.db.yandex-team.ru/api/v1.0/project/0/cluster/cbef5b21-16fa-4c6a-9b2f-f00b9ccc4b22" --header "Authorization: OAuth MY_OAUTH_TOKEN"
  {
    "cluster_type": "clickhouse_cluster", 
    "environment": "qa", 
    "id": "cbef5b21-16fa-4c6a-9b2f-f00b9ccc4b22", 
    "name": "gencfg_test1", 
    "options": {
      "clickhouse": {}, 
      "database": {
        "name": "gencfg_test1"
      }, 
      "flavor": "db1.nano", 
      "hosts": {
        "man-hb38oz0anr103epd.db.yandex.net": {
          "geo": "man"
        }, 
        "vla-m80v5zqpobp0gv6s.db.yandex.net": {
          "geo": "vla"
        }
      }, 
      "users": [
        {
          "name": "kimkim"
        }
      ], 
      "volume_size": 10737418240
    }
  }
  kimkim@minos:~/work/gencfg.svn(:trunk)$

После этих нехитрых манипуляций мы получили новую базу, которая доступна по адресам **man-hb38oz0anr103epd.db.yandex.net** и **vla-m80v5zqpobp0gv6s.db.yandex.net** . Из консоли до нее можно достучаться примерно так (вспоминая логин и пароль, который мы указали в начальной конфигурации):

::
  $ clickhouse-client --host man-hb38oz0anr103epd.db.yandex.net --port 9000 --user kimkim --databae gencfg_test1 --password aevohngaC0fahchaeh3yohp7eic
  ClickHouse client version 1.1.54310.
  Connecting to man-hb38oz0anr103epd.db.yandex.net:9000 as user kimkim.
  Connected to ClickHouse server version 1.1.54327.
  :)

Для обычного доступа (из приложения) нужно испольpовать порт 8123 .
