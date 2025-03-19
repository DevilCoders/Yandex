## Set up

1. Create a new projectId
1. Create a new cloud and network
1. Set up [a new public DNS zone](https://forms.yandex-team.ru/surveys/71190/) (e.g. iam-bot.yandexcloud.net)
1. [Issue a new certificate](https://cloud.yandex.com/en-ru/docs/certificate-manager/operations/managed/cert-create/) (e.g. iam-bot.yandexcloud.net)
1. Create a new YDB serverless database
1. Run `database/schema.sql`
1. Create a new SA, grant ydb.editor to the folder with the YDB
1. Create a new instance (e.g. using [the PaaS image](https://bb.yandex-team.ru/projects/CLOUD/repos/paas-images/browse/paas-base-g4/CHANGELOG.md/))
   ```
   $ yc compute instance create --name iam-bot \
       --folder-id <folder_id> \
       --zone ru-central1-a \
       --create-boot-disk image-id=<image_id> \
       --cores=2 --memory=4G --core-fraction=100 \
       --service-account-id=<service_account_id>
       --network-interface subnet-id=<subnet_id>,ipv6-address=auto,ipv4-address=auto \
   ```
1. Create a new L7 balancer:
   - healthcheck path `/heathcheck`
   - external port `telegram.webhook.port`
   - use the created certificate and domain
   - backend group port `telegram.webhook.listen.port`

1. Create [a new staff robot](https://wiki.yandex-team.ru/tools/support/zombik/)
1. Log in and create [a new OAuth application](https://oauth.yandex-team.ru/) with the following permissions:
   - staff.yandex-team.ru: Чтение данных со Стаффа
   - Стартрек: Запись в Стартреке, Чтение в Стартреке
1. Get the OAuth token https://oauth.yandex-team.ru/authorize?response_type=token&client_id=<application_id>
1. Request [a new SIM card](mailto:hr-mobile@yandex-team.ru)
1. Create [a new Telegram bot](https://core.telegram.org/bots#3-how-do-i-create-a-bot)
1. Create config.yaml
1. Make a bot executable
   ```
   $ ya make
   ```
1. Start the bot
   ```
   $ iam-bot --config config.yaml
   ```

## Bots

### Yandex Cloud Production
- folder b1getdl8d9bb8j372ue0
- endpoint iam-bot.yandexcloud.net
- bot name @ycloud_iam_bot
- phone [+79267327877](https://st.yandex-team.ru/MOBILE-386095/), owned by @potamus
- config prod.yaml

## Локальный запуск

1. Положить в config/robot-yc-iam-bot.token токен из https://yav.yandex-team.ru/secret/sec-01ef4rpbdmkw25ztvhe9rmxf4f/
2. Положить в config/access_log_viewer.password пароль из https://yav.yandex-team.ru/secret/sec-01fe890pmjrs07p9482zetacgh/
3. Создать базу данных https://console.cloud.yandex.ru/folders/b1gu1kk85jlt7bm5dmps (создать ресурс)) и скопировать endpoint в config/prod.yaml в database в endpoint, скопировать database в config/prod.yaml в database в database.
4. взять токен для ydb (https://yql.yandex-team.ru/oauth) и положить в переменную окружения
```bash
export YDB_ACCESS_TOKEN_CREDENTIALS=AQAD-...
```
5. Положить в config/tvm_application_secret.password секрет https://yav.yandex-team.ru/secret/sec-01g7yae62y6akkck1krpvqa68w/explore/version/ver-01g7yae639ta84xxmbawfnk8tb
6. Создать пустой файл config/ycloud_iam_bot.token
7. Запустить приложение:
```bash
ya make && ./telebot/iam-bot -c config/prod.yaml --local
```
8. Как-то общаться с ботом get и post запросами (изучить как это делает библиотека telethon и делать так же)

## Если нужно будет перевести обращение к стартреку на tvm

1. Найти tvm приложение трекера (какой-то из ресурсов https://abc.yandex-team.ru/services/STARTREK/resources/?view=consuming&layout=table&supplier=14&type=47) и скопировать там id.
2. Запросить доступ tvm приложения к трекеру https://wiki.yandex-team.ru/staff/staffroles/#rolnadostupkdannymvstaff-api (id приложения 2036156 (https://abc.yandex-team.ru/resources/?search=YC%20IAM%20Telegram%20bot&state=requested&state=approved&state=granting&state=granted))
3. Заменить класс ```OAuthClient``` на ```TvmClient``` в clients/startrek.py и создание ```Startrek``` и ```TVM2``` в main.py по аналогии со ```Staff```.

## Если нужно будет герметично тестировать бота (без обращения к telegram api)
1. Запускать приложение по инструкции локального запуска.
2. В тестах использовать внутренние инструменты используемой библиотеки pyTelegramBotAPI: ```handler``` методы и ```process_new_messages``` метод.
Пример есть в самой  библиотеке https://github.com/eternnoir/pyTelegramBotAPI/blob/master/tests/test_telebot.py
