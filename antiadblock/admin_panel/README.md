[![oko health](https://badger.yandex-team.ru/oko/repo/AntiADB/antiadblock-front/health.svg)](https://oko.yandex-team.ru/repo/AntiADB/antiadblock-front)
# AntiAdBlock frontend


## Начало работы
1. Ставим зависимости ``npm install && npm run local-deps``
2. Включает прекоммит-хуки ``npm run git-hooks``
3. Запускаем magicdev ``npm run magic``

## Доступные команды
``npm start`` запуск приложения на localhost

``npm run local-deps`` установка зависимостей для локальной работы, вынесены, чтоб не грузить лишнее на стендах

``npm run update-local-deps`` т.к. просто ``npm i`` удаляет зависимости, которые считаются лишними, можно пользоваться этой командой для обновления зависимостей

``npm run magic`` запуск приложения в режиме локальной разработки (см. magicdev)

``npm run build`` сборка проекта в production режиме (``NODE_ENV=production``)

``npm run local-prod`` запуск production сборки

``npm run lint`` запуск линтеров

``npm run jest`` запуск unit-тестов

``npm run test`` запуск линтеров и unit-тестов

``npm run i18n`` скачивание переводов из танкера

## Правила ведения репозитория

### Наименования
* название фича-ветки должно содержать краткое описание фичи и ID тикета, если тикет есть. Например, `ANTIADB-568-configs-list`.
* если для ветки нет соостветствующего тикета (правка настолько маленькая, что тикет не нужно заводить) или ветка не предполагает коммитов в неё (например, создана только для поднятия демки), то в её имени должен быть ник её автора. Например, `artyom-void-readme-update`.
* фича-ветку нужно удалять после мержа соответствующего PR'а. Свои устаревшие ветки нужно удалять.
* коммиты в PR'е нужно сквошить. Если изменений слишком много для одного коммита -- можно сквошить в несколько (но не очень много) атомарных, логически законченных коммитов.
* коммит-месседжи должны быть информативными и содержать ID тикета, если он есть. Правильно: `ANTIADB-544: Актуализировать README.md `. Неправильно: `ANTIADB-123` или `fix test 2`.

## Tanker
Для пулла переводов мы используем ``tanker-kit@1.6.8``, конфиг которого находится в .tanker/config.js

Добавить новые переводы можно [тут](https://tanker-beta.yandex-team.ru/project/antiadb)

Для пулла использовать ``npm run i18n``

## Arcadia CI
Живем на Arcadia CI @ https://docs.yandex-team.ru/ci/.
Найти наши релизы можно тут.
https://a.yandex-team.ru/projects/antiadblock/ci/releases/timeline?dir=antiadblock%2Fadmin_panel&id=release
Выкатки в preprod.
https://a.yandex-team.ru/projects/antiadblock/ci/actions/launches?dir=antiadblock%2Fadmin_panel&id=preprod-action
Выкатки на dev.
https://a.yandex-team.ru/projects/antiadblock/ci/actions/launches?dir=antiadblock%2Fadmin_panel&id=dev-action

LXC контейнер с [docker & nodejs & LFS](https://github.yandex-team.ru/search-interfaces/trendbox-ci/blob/master/docs/formats/0.2/recipes/build-lxc.md) можно найти в SB: [таска](https://sandbox.yandex-team.ru/task/885738703/view) и [ресурс](https://sandbox.yandex-team.ru/resource/1967268430/view)

#### Текущие секреты

Секреты лежат в [секретнице](https://yav.yandex-team.ru/?tags=antiadb)

Для деплоя добавлен DEPLOY_TOKEN пользователя robot-antiadb

Доступы до реджистри живут теперь [отдельно](http://docs.drone.io/manage-registry-credentials/).
Они нужны для того, чтобы дрон смог скачать плагин из внутреннего реджистри.

Добавлены так:
```
drone registry add --hostname registry.yandex.net --username qamsomolka --password <OAuth token from https://wiki.yandex-team.ru/cocaine/docker-registry-distribution/#avtorizacija> AntiADB/antiadblock-front
```

## Стенды
На каждый пулл-реквест собранный docker-образ деплоится на develop - https://develop.antiblock.yandex.ru/,
то есть стенд поднят только для последнего коммита

## Стейджи в deploy
* [Описание и редактирование стейджей](docs/deploy.md)

## Production сборка локально
1. Ставим зависимости
2. Ждем сборку ``npm run build``
3. Запускаем ``npm run local-prod``

## Флоу релиз

1. Создать релизный тикет в очереди ANTIADB. Вставить в описание тикета коммиты которые попадут в релиз. Найти можно по [ссылке](https://a.yandex-team.ru/projects/antiadblock/ci/releases/timeline?dir=antiadblock%2Fadmin_panel&id=release).
2. Проверить, что все работает на [препрод стейдже](https://preprod.antiblock.yandex.ru/).
3. Запустить сборку релиза нажав кнопку "Run release". [Ссылка на CI](https://a.yandex-team.ru/projects/antiadblock/ci/releases/timeline?dir=antiadblock%2Fadmin_panel&id=release).
4. Проверить, что [продакшен](https://antiblock.yandex.ru/) доступен.
5. Если не доступен, откатить production окружение в [deploy'е](https://deploy.yandex-team.ru/stage/antiadb-front-production).
6. Пройтись по привязанным к релизному тикету тикетам и закрыть их со статусом "fixed".
7. Закрыть релизный тикет.
8. Написать в общий чат со ссылкой на релизный тикет.
