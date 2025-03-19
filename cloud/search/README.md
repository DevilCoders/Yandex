# Yandex.Cloud Search

## Intro

Основные функции:

* Поставка документов о ресурсах в пользовательских облаках непосредственно для поиска;
* Поставка документов для индекса в Yandex Monitoring.

Облака: name - yc-search, id - yc.search.

### Поставка данных

preprod: https://logbroker-preprod.cloud.yandex.ru
prod: https://logbroker.cloud.yandex.ru
аккаунт: `yc.search`

топики:

* search-infrastructure - Compute, NBS, VPC, Load balancer, marketplace;
* search-data-services - MDB, Data Proc;
* search-dev-services – K8S, Functions, Container Registry, Instance Groups;
* search-cloud-native-services – YDB, DataLens;
* search-access - IAM, resource manager, KMS;
* search-s3 – S3;
* search-ymq - YMQ.

До появления Logbroker в Облаке использовался logbroker.yandex-team.ru, в нем `yandexcloud/preprod/*`, `yandexcloud/prod/*` с аналогичным набором топиков. Поставка через эту инсталляцию уже задепрекейчена.

Сделано так, чтобы при появлении poison message встанет индексация всех документов за ним, в т.ч. и других сервисов. Но при этом поисковый backend технически ограничен количеством топиков, которые нужно вычитывать, а потому нельзя создать по топику на сервис. Соответсвенно поставка разбита на топики для группы близких сервисов (условно разрабатываемых одной командой).

Схема документов - [a:trunk/arcadia/cloud/search/schemas/v2/event.json](https://a.yandex-team.ru/arc/trunk/arcadia/cloud/search/schemas/v2/event.json)
По этой схеме валидируется поставка и вычитка из топиков.

### Шаги заезда сервисов в Поиск

1. Создание поставщиков (и потребителей для отладки) в LogBroker. [Дока](https://logbroker.yandex-team.ru/docs/).
2. Для доступов к топикам, обычно ReadTopic + WriteTopic + ReadAsConsumer, можно создать тикет в st/CLOUD, компонент Search с перечислением какие топики для каких клиентов нужно награнтить права.
3. Начинаете поставку данных, валидируясь по схеме.
4. Заводите тикет st.yandex-team.ru/PS с компонентой "Поиск Яндекс.Облако", в фоллверы vonidu@, asalimonov@ и с описанием какие дополнительные атрибуты вы хотите использовать в поставляемых документах и из какого топика.
5. После подключения бэкенда от PS нужен тикет в st.yandex-team.ru/CLOUDFRONT с вашим компонентом, в фолловеры seqvirioum@, чтобы добавили. Вам помогут с дизайном и если нужно нарисуют иконку.

Чтобы задача не терялась, следует ее прилинковать к  [CLOUD-25714 - Яндекс.Облако: Поиск](https://st.yandex-team.ru/CLOUD-25714).

### TBD (заполнить)

* Борды мониторинга per-service в Solomon;
* Разложить схемы документов от каждого сервиса в Аркадии для поискового бэкенда.