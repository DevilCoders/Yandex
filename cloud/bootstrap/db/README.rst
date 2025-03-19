============
Bootstrap DB
============

Данные храним в `PGaaS <https://doc.yandex-team.ru/cloud/managed-postgresql/index
.html>`_ . Текущая выделенная база `ТУТ <https://yc.yandex-team.ru/folders/foospamdce0ds1n151uq/managed-postgresql/cluster/mdbu8jrerq3d2099jh2q>`_ .


Структура репозитория
---------------------

Стрктура репозитория (сгенерировано `dd`) выглядит следующим образом:
::

    .
    ├── bin
    │   └── admin  # админская утилита
    ├── data  # вспомогательные данные (конфиги)
    │   └── local  # конфиги для подъема локальной базы)
    ├── packages  # генерируемые пакеты
    │   └── yc-bootstrap-db-admin  # пакет с админкой
    └── schema  # схемы
        ├── 0.0.1
        ├── 0.1.0
        └── current  # текущая схема

Начало работы
-------------

На всякий случай стоит настроить админский CLI и доступ через psql, как написано в `документации <https://doc
.yandex-team.ru/cloud/managed-postgresql/quickstart.html>`_.

Проверяем, что работает CLI:

::

  kimkim@kimkim-desktop:~$ yc managed-postgresql cluster list
  +----------------------+---------+---------------------+--------+---------+
  |          ID          |  NAME   |     CREATED AT      | HEALTH | STATUS  |
  +----------------------+---------+---------------------+--------+---------+
  | mdbu8jrerq3d2099jh2q | kktest1 | 2019-07-11 16:28:21 | ALIVE  | RUNNING |
  +----------------------+---------+---------------------+--------+---------+
  kimkim@kimkim-desktop:~$


Проверяем что работает клиент psql (пароль берем в `секретнице <https://yav.yandex-team.ru/secret/sec-01dfkaxd90gnd02pqb85r9eyey/explore/versions>`_):

::

  kimkim@kimkim-desktop:~$ psql "host=sas-m1ay12l8u6ge9ggg.db.yandex.net port=6432 sslmode=verify-full dbname=db1 user=user1 target_session_attrs=read-write" -c "SELECT datname FROM pg_database WHERE datistemplate = false"
   datname
  ------------
   db1
   postgres
   (2 rows)


Подъем локальной базы
---------------------

Для тестирования логично поднять локальную базу. Последовательность действий следующая для подъема базы с использованием docker-а (на машине должны быть пакеты **docker.io**, **docker-compose**, и у пользователя должны быть права на использование docker-а (проще всего проверить, что работает **docker ps**)).

#. Инициализируем базу командой **bin/admin/bootstrap.db.admin start-db-in-docker --populate-current-db** (в этом случае будет использована схема из **schema/current/bootstrap.sql**).
#. Проверяем что база нормально инициализирована командой **bin/admin/bootstrap.db.admin status --db-config bin/admin/localdb.yaml**
#. Заходим в консоль либо через **bootstrap.db.admin** командой **bin/admin/bootstrap.db.admin console --db-config bin/admin/localdb.yaml**, либо через **psql** командой **psql "postgres://postgres@localhost:12000/postgres?sslmode=disable"**

После завершения работы нужно не забыть опустить локальную базу (через **docker ps** находим правильный контейнер и через **docker stop** останавливаем его).
