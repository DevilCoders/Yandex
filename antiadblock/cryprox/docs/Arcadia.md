## Разработка в Аркадии

До начала разработки надо ознакомиться с [инструкцией](https://wiki.yandex-team.ru/arcadia/starterguide/)
Там установка `ya`.

1. [Тестирование](#тесты)
1. [Ревью](#review-request)
1. [Автоматическая сборка](#автоматическая-сборка)
1. [Собрать контейнер из Review request](#как-собрать-докер-контейнер-из-review-request)
1. [Отладка](#отладка)

### Выкачать и собрать наш проект:

`ya make --checkout antiadblock/cryprox`

Команды можно выполнять из корня Аркадии

### Тесты
Запустить тесты и проверки можно локально:
`ya make -tt --checkout antiadblock/cryprox` - запустить все тесты (юниты, доктесты, функциональные)  
`ya make -tt --checkout antiadblock/cryprox/tests/functional` - запустить все функциональные тесты  
`ya make -tt --checkout antiadblock/cryprox/tests/unit` - запустить все юнит тесты  
`ya make -tt --checkout antiadblock/cryprox/cryprox` - запустить все doc тесты  
`ya make -tt -F "test_name" --checkout antiadblock/cryprox` - запустить тесты, подходящие под фильтр "test_name"  
`ya make -tt -F "test_name.py::concreete_test*" --checkout antiadblock/cryprox` - запустить тест `concreete_test` из файлика `test_name.py`  
`ya make -tt --tests-retries=3 --checkout antiadblock/cryprox` - запустить тесты с 3 ретраями (для поиска флапающих тестов)   
`ya make -tt --pdb --checkout antiadblock/cryprox` - запустить тесты с остановкой в дебаггере

Больше примеров: https://wiki.yandex-team.ru/yatool/test/#lokalnyjjzapusktestov

#### Как посмотреть логи тестов

Команда `ya make -tt --checkout cryprox/tests` по умолчанию кладет результаты тестов в `cryprox/tests/test-results`. 
Там можно найти логи последнего запуска: инициализация, упавшие тесты, обычный stdout, логи бинаря cryprox_run
Если вы запускали конкретный тест, например, какой-то функциональный, то его логи будут в директории `cryprox/tests/functional/test-results`.
 
### Review request:

Вся разработка cryprox должна проходить через ревью. 

Для того, чтобы аркадийный хук создал новое ревью, нужно чтобы в сообщении к коммиту встретилось волшебное сочетание букв REVIEW:NEW (расположение неважно)

`svn commit -m "added new cool feature REVIEW:NEW"  <file1> <file2>`

Или через утилиту [ya](https://clubs.at.yandex-team.ru/arcadia/17804):

`ya pr c -m "added new cool feature"`

Будет выведена ссылка на ревью в Аркануме

`> Your commit data has been uploaded into the review system: https://a.yandex-team.ru/arc/review/558946`

Подробнее о ревью [здесь](https://wiki.yandex-team.ru/arcadia/review/)

В начале сообщения к коммиту нужно указывать номер тикета в стартреке. К ревью в `description` нужно обязательно прилагать полное описание того, что сделано на русском языке. 

Хорошо: 
```
переименование параметра pull_docker_url на cryprox_docker_url в ansible сценариях:
ansible/roles/cryprox/meta/main.yml
```

Плохо: 
```
исправить замечания
```

### Автоматическая сборка

Докер-образы собираются автоматически по появлению коммита в транке (отслеживаются пути `antiadblock/cryprox/cryprox` и `antiadblock/cryprox/cryprox_run`). 

Тэг собранного образа равен ревизии коммита. Статус нужно смотреть в БД `testenv`.
Документация [Testenv](https://wiki.yandex-team.ru/testenvironment/).

Собранный образ помечается тегом `registry.yandex.net/antiadb/uni:{revision}`, и кладется в Яндексовый docker registry. </br>
Статус можно смотреть в [Джоба сборки](https://beta-testenv.yandex-team.ru/project/antiadblock/job/BUILD_ANTIADBLOCK_UNI_PROXY_DOCKER)


### Как собрать докер-контейнер с патчем
Контейнер собирается таской `YA_PACKAGE`. Нужно запустить ее со следующими параметрами:</br> 
(Для простоты можно склонирoвать [эту](https://sandbox.yandex-team.ru/task/925881150/view))
* Так как таска собирает и пушит образ как `registry.yandex.net/<repository>/<package name>:<package version>`, то в патче нужно 
отредактировать свойство `version` в файле `arcadia/antiadblock/cryprox/package.json`
* Из корня аркадии выполнить команду для создания диффа `ya tool svn diff --patch-compatible antiadblock/cryprox > cryprox.diff`
* В файле `cryprox.diff` сохранится дифф
* Склонировать (или создать) таску YA_PACKAGE
* В поле `Svn url for arcadia` указать ревизию `arcadia:/arc/trunk/arcadia@{revision}`, revision взять из диффа
* В поле `Apply patch` вставить патч (все варианты применения патча описаны [тут](https://nda.ya.ru/3QTTV4))
* В `Package paths` указать путь `antiadblock/cryprox/package.json`
* `Package type: debian or tarbal`: `docker`, `Save docker image in resource`: `True`,  `Push docker image`: `True`, 
`Docker registry`: `registry.yandex.net`, `Docker user`: `robot-antiadb`, `Docker token vault name` : `ANTIADBLOCK_REGISTRY_ACCESS_KEY`
 

### Отладка

`pdb.set_trace()` работает. `pytest.set_trace()` работает с такими ограничениями: запускать надо бинарник с тестами, который `ya make -t` создаст в папке с тестами.
Запукать бинарник с тестами надо из этой папки.
