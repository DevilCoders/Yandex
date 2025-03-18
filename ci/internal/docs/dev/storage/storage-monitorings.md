# Мониторинги Storage

## Dashboard

[Solomon](https://solomon.yandex-team.ru/?project=ci&cluster=stable&dashboard=ci_storage)

## Сервисы в Yandex Deploy

### Stable

[Solomon](https://solomon.yandex-team.ru/?project=ci&cluster=stable&dashboard=ci_storage)

Сервисы в Deploy:

* [Shard](https://deploy.yandex-team.ru/stages/ci-storage-shard-stable)
* [Reader](https://deploy.yandex-team.ru/stages/ci-storage-reader-stable)
* [API](https://deploy.yandex-team.ru/stages/ci-storage-api-stable)
* [TMS](https://deploy.yandex-team.ru/stages/ci-storage-tms-stable)

### Prestable

[Solomon](https://solomon.yandex-team.ru/?project=ci&cluster=prestable&dashboard=ci_storage)

Сервисы в Deploy:

* [Shard](https://deploy.yandex-team.ru/stages/ci-storage-shard-prestable)
* [Reader](https://deploy.yandex-team.ru/stages/ci-storage-reader-prestable)
* [TMS](https://deploy.yandex-team.ru/stages/ci-storage-tms-prestable)

### Testing

[Solomon](https://solomon.yandex-team.ru/?project=ci&cluster=testing&dashboard=ci_storage)

Сервисы в Deploy:

* [Shard](https://deploy.yandex-team.ru/stages/ci-storage-shard-testing)
* [Reader](https://deploy.yandex-team.ru/stages/ci-storage-reader-testing)
* [TMS](https://deploy.yandex-team.ru/stages/ci-storage-tms-testing)

## Мониторинги по чтению из логброкера

* [Stable](https://juggler.yandex-team.ru/check_details/?host=ci-storage-stable&service=logbroker)
* [Prestable](https://juggler.yandex-team.ru/check_details/?host=ci-storage-prestable&service=logbroker)
* [Testing](https://juggler.yandex-team.ru/check_details/?host=ci-storage-testing&service=logbroker)

Топики [тут](https://logbroker.yandex-team.ru/lbkx/accounts/ci/autocheck), разложены по средам.

Мониторинг включает в себя две проверки:

* storage-{cluster}-lb-lag - Время с последнего чтения.
* storage-{cluster}-last-read - Отставание чтения.

Обычно срабатывают парой, означают, что сервис не успевает вычитывать или не читает совсем свои топики.

### Посмотреть графики коммитов

Проверить по графикам not commited на [борде](#Dashboard)
или [общий](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=storage-shard%7Cstorage-reader&l.host=!cluster&l.sensor=storage_shard_in_lb_not_commited_reads&graph=auto&stack=false)
, что нет повисших read, они выглядят как горизонтальная линия на графике одного или более хостов. Надо читать логи на
хосте, где есть зависшие read.

### Посмотреть мониторинг и графики ошибок

Reader errors и Shard errors на [борде](#Dashboard)
или [общий](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=storage-shard%7Cstorage-reader&l.host=cluster&l.sensor=storage_errors_total&graph=auto&hideNoData=true)
. Если есть ошибки, пойти в логи сервиса и посмотреть, что за ошибки. Если ошибок нет, смотреть на общий перфоманс и
загруженность YDB.

### Посмотреть график обработки результатов

Проверить
на [графике](https://solomon.yandex-team.ru/?project=ci&cluster=stable&service=storage-shard&l.host=!cluster&l.sensor=storage_results_processed_total&graph=auto&stack=false&numberFormat=3%7CK)
, какой шард обрабатывает меньше всего результатов или не обрабатывает вовсе. Дальше посмотреть на нем логи.

## Мониторинги ошибок

* [Stable](https://juggler.yandex-team.ru/check_details/?host=ci-storage-stable&service=errors)

Мониторинг включает в себя две проверки:

* storage-stable-shard-errors - Ошибки шарда.
* storage-stable-reader-errors - Ошибки ридера.

Нужно смотреть, что за ошибки на графике, затем идти смотреть логи на хосте, на котором возникают ошибки.

### Типы ошибок

* aggregate_error, chunk_error, aggregate_report_error - ошибки при обработке соответствующей очереди, детали в логах.
* aggregate_save_error - ошибка при сохранении агрегата, обычно возникают из-за перегрузки YDB и успешно ретраятся.
* %stream%_lb_commit - не получилось закоммитеть read из Logbroker, критично, если возникает не при выкладке сервиса.
* %stream%_lb_read - ошибка обработки read из Logbroker, детали в логах.
* %stream%_missing - не найден объект, по которому пришли результаты.
* %stream%_parse_error - ошибка парсинга данных из Logbroker.
* %stream%_validation - ошибка валидации данных из Logbroker, означает что пришли некорректные или неполные данные.
* %stream%_write - ошибка записи данных в поток.

%stream% - поток данных, есть:
* main - главный входной поток от автосборочных задач.
* shard_in - входной поток шарда.
* shard_out - выходной поток шарда.

## Мониторинг api

* [Stable](https://juggler.yandex-team.ru/check_details/?host=ci-storage-stable&service=api)
* [Prestable](https://juggler.yandex-team.ru/check_details/?host=ci-storage-stable&service=api)
* [Testing](https://juggler.yandex-team.ru/check_details/?host=ci-storage-stable&service=api)

Показывает наличие ошибок на ручках api. При наличии ошибок, надо смотреть график, какие именно запросы не проходят и
логи сервиса. Ошибки по ручкам созданиям проверки критичны, они замедляют или останавливают автосборку.

## Мониторинг YDB

* [Stable](https://juggler.yandex-team.ru/check_details/?host=ci-storage-ydb-stable&service=ydb)
* [Prestable](https://juggler.yandex-team.ru/check_details/?host=ci-storage-ydb-prestable&service=ydb)
* [Testing](https://juggler.yandex-team.ru/check_details/?host=ci-storage-ydb-testing&service=ydb)

[Документация YDB](https://ydb.yandex-team.ru/docs/maintenance/monitoring)
