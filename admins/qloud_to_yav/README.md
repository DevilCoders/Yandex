# Дампер секретов Qloud->Yav с гибким acl

Позволяет сдампить все секреты из Qloud и положить их в Yav сразу с правильным acl. Такого не позволяет сделать yd-migrate.

Нужно иметь в переменных окружения `YAV_TOKEN` -- ВАШ токен от секретницы. Получить: https://nda.ya.ru/3UVsAN, и `QLOUD_API_TOKEN` -- токен от кулауда. Получить: https://nda.ya.ru/t/Gv8ooK8q3VuHEV

Параметры.

* `create` -- создать секрет
* `--sec-name` -- имя секрета
* `--allow-rw-users` -- логины пользователей, котрых добавить в RW секцию акл секрета. Перечислять через пробел без кавык и собак. Например: `coldmind vyacheslav`
* `--allow-ro-users` -- то же самое, только для RO. Только на чтение.
* `--allow-rw-abc` -- расшарить доступ RW на определённый abc скоуп. Задаётся как id abc сервиса и скоуп. id это int, а скоуп это str. Например: `645,administration`. Это будет [Кинопоиск(Администрирование)](https://abc.yandex-team.ru/services/kp/?scope=administration). Если нужно задать несколько скоупов, то это делается так: `645,administration 645,development`. Это будет [Кинопоиск(Администрирование)](https://abc.yandex-team.ru/services/kp/?scope=administration) и [Кинопоиск(Разработка)](https://abc.yandex-team.ru/services/kp/?scope=development)
* `--allow-ro-abc` -- то же самое, только для RO.
* `--qloud-component` -- компонента, её полный путь вида `проект.приложение.окружение.компонента`. Например: `kinopoisk.film-list-api.production.film-list-api`
* `--qloud-environment` -- окружение, его полный путь вида `проект.приложение.окружение`. Например: `kinopoisk.film-list-api.production`
* `--use-prod-yav` -- флаг для использования продакшен секретницы (https://yav.yandex-team.ru/).
* `--force-create` -- флаг для создания пустого сектера, если в компоненте нет секретов.
* `--suppress-resolv-fail` -- флаг для обрабатывания крайнего случая, о нём ниже рассказано подробно.

### Компонент либо окружение
Может быть задан либо компонент (`--qloud-component`), либо окружение (`--qloud-environment`).
Если задаётся компонент, дампятся и кладутся в секрет секреты только компонента.
Если задаётся окружение, дампятся и кладутся в секрет секреты всех компонент окружения. Это бывает полезно, когда хочется сделать секрет на приложение, в котором много компонент.

!!!
При этом, бывает, что одна из компонент пустая (без задеплоеных контейнеров). Скрипт будет падать с ошибкой, но если заведомо известно, что есть такая компонента, то можно воспользоваться флагом `--suppress-resolv-fail`, и сдампятся секреты только из не пустых компонент.
Так же если в компоненте вообще нет секретов, секрет НЕ создастся, а просто выведется варнинг. Чтобы содать пустой секрет для компоненты, в которой нет секретов, нужно использовать флаг `--force-create`

### Тестинг или прод секретницы
По умолчанию скрипт будет использовать тестовую версию секретницы (https://yav-test.yandex-team.ru/). Чтобы заливать секреты в прод, нужно использовать флаг `--use-prod-yav`.
Для начала чтобы поиграться со скриптом, лучше использовать тестовую секретницy. Дальше, чтобы пушить секреты в прод, это надо будет явно указать аргументом.

### Только посмотреть секреты и не пушить в секретницу
Если не указать никаких флагов, связанных с секретом, а только create и component|environment, то содержимое секретов вместе с айдишнками и наименованиями переменных выведется в stdin.

### Работа с файлами
Скрипт поддерживает работу с файлами. Если в окружении определён секрет в качестве файла, то он загрузит его в Yav тоже в виде файла. У `yd-migrate` реализована загрузка файла в Yav в виде плейн текста, что не удобно. Работает это за счёт наличия в системе пользователя утилиты `ya`. Так как, в Python API vault не реализована работа с файлами (https://t.me/c/1236844232/30).


### Пример использования №1
Хотим сдампить секреты компонентa `quiz-and-questionnaire-api` в окружении `kinopoisk.quiz-and-questionnaire.testing`
![](https://jing.yandex-team.ru/files/coldmind/2020-10-23-011954_1804x898_scrot.png)

Компонент имеет 2 секрета:
![](https://jing.yandex-team.ru/files/coldmind/2020-10-23-012017_1816x466_scrot.png)

Хотим, чтобы редактировать секрет могли coldmind@, vyacheslav@, и Кинопоиск(Разработка). А читать мог robot-kp-java@.
```
» ./qloud_to_yav.py create --sec-name quiz --allow-rw-users coldmind vyacheslav --allow-ro-users robot-kp-java --allow-rw-abc 645,development --qloud-component kinopoisk.quiz-and-questionnaire.testing.quiz-and-questionnaire-api

https://yav-test.yandex-team.ru/secret/sec-01en96x4grdxspgyfekt13pv5m
```
Итоговый результат:
upd: В текущей версии скрипта, в названиях ключей секретов, не будет кулаудного `secret.`, он будет обрезаться, если есть. Так как это лишее.

![](https://jing.yandex-team.ru/files/coldmind/2020-10-23-013015_1415x715_scrot.png)



### Пример использования №2 -- с крайним случаем.
Хотим сдампить секреты всех компонент окружения `production` в приложении `kinopoisk.film-list-api`
![](https://jing.yandex-team.ru/files/coldmind/2020-10-23-012038_1811x901_scrot.png)

Крайний случай: видим, что компонент jobs раздеплоен и в нём нет инстансов. Поэтому добавить надо флаг `--suppress-resolv-fail`
Компонент имеет 1 секрет:
![](https://jing.yandex-team.ru/files/coldmind/2020-10-23-012051_1818x407_scrot.png)

Хотим, чтобы редактировать секрет могли coldmind@, vyacheslav@, и Кинопоиск(Разработка). А читать мог robot-kp-java@.
```
» ./qloud_to_yav.py create --sec-name film-list-api --allow-rw-users coldmind vyacheslav --allow-ro-users robot-kp-java --allow-rw-abc 645,development --qloud-environment kinopoisk.film-list-api.production --suppress-resolv-fail

https://yav-test.yandex-team.ru/secret/sec-01en96zgd6v9fb4vn0rs1btexz
```

Итоговый результат:

![](https://jing.yandex-team.ru/files/coldmind/2020-10-23-013117_1413x746_scrot.png)

### Пример использования №3 -- в окружении есть секреты в виде файлов
Хотим сдампить все секреты компонента `quiz-and-questionnaire-api`.
В целях демонстрации добавил туда файл `phpsecrets`.
![](https://jing.yandex-team.ru/files/coldmind/2020-10-23-150850_1480x472_scrot.png)

Запускаем,
```
» ./qloud_to_yav.py create --sec-name quizzezzz --allow-rw-users coldmind vyacheslav --allow-ro-users robot-kp-java --allow-rw-abc 645,development --qloud-component kinopoisk.quiz-and-questionnaire.testing.quiz-and-questionnaire-api
https://yav-test.yandex-team.ru/secret/sec-01enan7b5qn9nmf07139f59he5
```

Результат

![](https://jing.yandex-team.ru/files/coldmind/2020-10-23-151017_1492x773_scrot.png)
