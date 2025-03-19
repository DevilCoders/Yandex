# Генератор дашборда

По вопросам обращаться к @arovesto

---

#### Инструкция по созданию

1. Создать дашборд в Графане:

   1. Зайти в [Grafana UI](https://grafana.yandex-team.ru), создать дашборд:
      - с помощью меню слева (выбрать `[+] → Create → Dashboard`)
      - в меню справа зайти в настройки: `⚙`
      - в меню настроек (слева) нажать `[Save]`

   2. Получить UID (его надо будет указать в yaml-конфигурации):
      - зайти в настройки: `⚙`
      - выбрать в меню слева `JSON Model`
      - найти среди полей (они упорядочены по алфавиту) поле `uid` и скопировать его в `specs.env` файл.

2. Скачать docker-образ с генератором:

   1. Получить OAUTH токен для докера [тут](https://oauth.yandex.ru/authorize?response_type=token&client_id=1a6990aa636648e9b2ef855fa7bec2fb), если еще нет.
   2. Залогиниться: `docker login --username oauth --password <OAUTH_TOKEN> cr.yandex`
   3. Скачать образ: `docker pull cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest`
   4. Тестовый запуск контейнера: `docker run --rm -it cr.yandex/crp6ro8l0u0o3qgmvv3r/dashboard:latest`

   Подробнее про [Yandex Container Registry](https://cloud.yandex.ru/docs/container-registry/operations/authentication)

3. Обновить конфигурацию дашборда:

    1. Если ничего не меняется в источниках данных то этот пункт можно пропустить.

    2. Параметры для дашбордов берутся из файла `specs.env`, там можно задать параметры создающихся дашбордов.

    3. Почему так?

    Потому что есть несколько Solomon источников (`Solomon`, `Solomon Cloud Preprod` и возможно другие). На данный момент нельзя менять источник при помощи переменной из дашборда, по этому для разных источников нужны разные дашборды.

4. Обновить сам дашборд:

    1. см. [README по генерации](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/java/dashboard/README.md) и [пост в этушке](https://clubs.at.yandex-team.ru/ycp/1677)
    2. Сделать необходимые изменения в дашборд файле `template.yaml`
    3. Обновить дашборд командой `ACTION=upload GRAFANA_OAUTH_TOKEN='your-token' ./dashboard.sh`

5. Отправить конфигурацию в Графану:

   1. Получить OAUTH токен для Графаны [тут](https://oauth.yandex-team.ru/authorize?response_type=token&client_id=cfa5d75ea95f4ae594acbdaf8ca6770c)
   2. Воспользоваться скриптом `dashboard.sh`:

       - Задать OAUTH токен для Графаны в переменной окружения `GRAFANA_OAUTH_TOKEN`
       - В переменной `ACTION` указать, что делать:

          - local (по умолчанию) - прогоняет генератор локально, проверяет ошибки, выводит полученный JSON файл спецификации
          - diff - сравнивает конфиг с тем, что в Графане, выводит различия
          - upload - обновляет дашборд в графане

   3. Просто обновить дашборд: `ACTION=upload GRAFANA_OAUTH_TOKEN='your-token' ./dashboard.sh`
   4. После обновления дашборда в выводе будет указана ссылка на него, лучше запускать с `&2>/dev/null` для упрощения поиска ссылок


