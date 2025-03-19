=============
Bootstrap API
=============

Bootstrap api started on ticket CLOUD-24725_


Repo layout
-----------
Repository layout (generated with `tree -d -L 3`) looks like this:
::

    .
    ├── bin  # directory with all binaries
    │   └── backend  # backend binary
    │       ├── routes  # api endpoints
    │       └── core  # business logic
    ├── common  # source code, shared accross all binaries as library
    ├── packages  # packages
    │   └── yc-bootstrap-api
    └── tests  # unit tests
        └── common  # tests on common


Local API backend
-----------------

Для тестирования логично поднять локальную базу. Последовательность действий следующая (для подъема базы с использованием docker-а).

#. Поднимаем локально базу как описано в **../db/README.rst** .
#. Запускаем api, который будет смотреть в эту базу: **bin/backend/bootstrap.api --config localdb.yaml**. В результате api поднимется локально на порту 23712.
#. Проверяем что api живой: в браузере смотрим, что есть страница с документацией - http://127.0.0.1:23712/doc , из консоли проверяем запросом **curl -X GET "http://127.0.0.1:23712/admin/version?pretty=1" -H  "accept: application/json"**


Функциональные тесты
--------------------

В каталоге **api/tests/bin/backend** находятся функциональные тесты для api. Работают они следующим образом: в специальном контейнере поднимается postgres, на него накатыается нужная схема. Далее поднимается api, который смотрит на эту базу и в тестах дергаются api-ые вызовы.

Тесты запускаются командой **ya make -ttt**. Для удобства тестирования можно сказать им, чтобы они не поднимали api, а использовали уже существующий (который должен быть запущен с конфигом **bin/backend/localdb.yaml**).

Тесты могут зависать (по пока не понятной причине). Если такое произошло, после прерывания тестов нужно обязательно остановить и ОБЯЗАТЕЛЬНО УДАЛИТЬ docker-контейнер с postgres командой **docker-compose -f tests/bin/backend/docker-compose.yml stop && docker-compose -f tests/bin/backend/docker-compose.yml rm -f**.



.. _CLOUD-24725: https://st.yandex-team.ru/CLOUD-24725
