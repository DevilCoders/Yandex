# Общие вопросы

## Что такое {{ mgp-short-name }}? {#what-is}

{{ mgp-short-name }} — это сервис, который помогает вам создавать, эксплуатировать и масштабировать базы данных {{ GP }} в облачной инфраструктуре.

С {{ mgp-short-name }} вы можете:

* создавать базы данных с необходимыми параметрами производительности;
* масштабировать вычислительные мощности и выделенный объем хранилища для баз данных по мере необходимости;
* получать журналы работы баз данных.

{{ mgp-short-name }} берет на себя трудоемкие задачи администрирования инфраструктуры {{ GP }}:

* предоставляет мониторинг потребляемых ресурсов;
* автоматически создает резервные копии баз данных;
* обеспечивает отказоустойчивость за счет автоматического переключения на резервные реплики;
* своевременно обновляет программное обеспечение СУБД.

Вы взаимодействуете с кластером БД в {{ mgp-short-name }} как с обычной базой данных в вашей локальной инфраструктуре. Благодаря этому вы можете управлять внутренними настройками БД в соответствии с требованиями вашего приложения.

## Какую часть работы по управлению и сопровождению баз данных берет на себя {{ mgp-short-name }}? {#services}

При создании кластеров {{ mgp-short-name }} выделяет ресурсы, устанавливает СУБД и создает базы данных.

Для созданных и запущенных баз данных {{ mgp-short-name }} автоматически создает резервные копии, а также устанавливает исправления и обновления СУБД.

Также {{ mgp-short-name }} обеспечивает репликацию данных между хостами БД (как внутри, так и между зонами доступности) и автоматически переключает нагрузку на резервную реплику в случае аварии.


## Для каких задач стоит использовать {{ mgp-short-name }}, а для каких виртуальные машины с базами данных? {#mdb-advantage}

{{ yandex-cloud }} предлагает два варианта работы с базами данных:

* {{ mgp-short-name }} позволяет вам эксплуатировать шаблонные базы данных, не заботясь об администрировании.
* Виртуальные машины {{ compute-full-name }} позволяют вам создавать и настраивать собственные базы данных. Такой подход позволяет использовать любые СУБД, подключаться к базам данных по SSH и так далее.


## Что такое хост базы данных и кластер базы данных? {#what-is-cluster}

_Хост БД_ — это изолированная среда базы данных в облачной инфраструктуре с выделенными вычислительными ресурсами и зарезервированным объемом хранилища данных.

_Кластер БД_ — это один или более хостов БД, между которыми можно настроить репликацию.


## Как начать работу с {{ mgp-short-name }}? {#quickstart}

{{ mgp-short-name }} доступен всем зарегистрированным пользователям {{ yandex-cloud }}.

Чтобы создать кластер базы данных в {{ mgp-short-name }}, необходимо определиться с его характеристиками:

* [Класс хостов](../concepts/instance-types.md) (характеристики производительности — процессоры, память и т. п.).
* Объем хранилища (резервируется в полном объеме при создании кластера).
* Сеть, к которой будет подключен ваш кластер.
* Количество хостов для кластера и зона доступности кластера.

Подробные инструкции см. в разделе [{#T}](../operations/cluster-create.md).


## Сколько хостов БД может содержать кластер? {#how-many-hosts}

Кластер {{ mgp-short-name }} состоит минимум из 4 хостов:

* 2 хоста-мастера;
* 2 хоста-сегмента.

Количество хостов-сегментов можно увеличить до 32.

Подробнее см. в разделе [{#T}](../concepts/limits.md).


## Как получить доступ к запущенному хосту базы данных? {#db-access}

Вы можете подключаться к базам данных {{ mgp-short-name }} способами, стандартными для СУБД.

[Подробнее о подключении к кластерам](../operations/connect.md).

## Сколько кластеров можно создать в рамках одного облака? {#db-limit}

Технические и организационные ограничения MDB приведены в разделе [{#T}](../concepts/limits.md).


## Как происходит обслуживание кластеров БД? {#service-window}

Под обслуживанием в {{ mgp-short-name }} понимается:

* автоматическая установка обновлений и исправлений СУБД для ваших хостов БД;
* изменение класса хостов и объема хранилища;
* другие сервисные работы {{ mgp-short-name }}.

Подробнее в разделе [{#T}](../concepts/maintenance.md).

## Что происходит, когда выпускается новая версия СУБД? {#new-version}

Программное обеспечение баз данных обновляется при выходе новых минорных версий. Владельцы затронутых кластеров БД получают предварительное оповещение о сроках проведения работ и доступности баз данных.


## Что происходит, когда версия СУБД становится неподдерживаемой (deprecated)? {#dbms-deprecated}

Через месяц после того, как версия СУБД становится неподдерживаемой, {{ mgp-short-name }} автоматически оповещает владельцев кластеров БД, созданных с этой версией, по электронной почте.

Создание новых хостов с СУБД неподдерживаемых версий становится невозможным. Через 7 дней после оповещения для минорных версий и через 1 месяц для мажорных версий проводится автоматическое обновление кластеров БД до следующей поддерживаемой версии. Обновление неподдерживаемых мажорных версий происходит даже если у вас отключено автоматическое обновление.


## Как рассчитывается стоимость потребления для хоста базы данных? {#db-cost}

В {{ mgp-short-name }} стоимость потребления рассчитывается исходя из следующих параметров:

* Выбранный класс хостов.
* Объем хранилища, зарезервированного для хоста БД.
* Объем резервных копий кластера БД. {% if audience != "internal" %}Объем резервных копий, равный объему хранилища, не тарифицируется. Хранение резервных копий сверх этого объема оплачивается по [тарифам](../pricing/index.md).{% endif %}
* Количество часов работы хоста БД. Неполные часы округляются до целого значения. {% if audience != "internal" %}Стоимость часа работы для каждого класса хостов приведена в разделе [{#T}](../pricing/index.md).{% endif %}


## Как изменить вычислительные ресурсы и объем хранилища для кластера БД? {#resources-change}

Вы можете изменять вычислительные ресурсы и объем хранилища в консоли управления — просто выберите другой класс хостов для нужного кластера.

Характеристики кластера изменяются в течение 30 минут. В этот период также могут быть включены другие сервисные работы по кластеру, например, установка обновлений.


## Включено ли резервное копирование хостов БД по умолчанию? {#default-backup}

Да, по умолчанию резервное копирование включено. Для {{ GP }} выполняется полное резервное копирование один раз в сутки, и сохраняются все журналы транзакций кластера БД. Это позволяет восстановить состояние кластера на любой момент времени в пределах периода хранения резервных копий, за исключением последних 30 секунд.

Резерные копии существующего кластера, созданные автоматически, хранятся 7 дней, а созданные вручную — бессрочно. После удаления кластера все резервные копии хранятся 7 дней.

## Когда выполняется резервное копирование? Доступен ли кластер БД во время резервного копирования? {#backup-window}

Окно резервного копирования — это интервал времени, в течение которого выполняется ежедневное полное резервное копирование кластера БД. Вы можете задать окно резервного копирования при создании и изменении кластера.

Во время окна резервного копирования кластеры остаются полностью доступными.

## За какими метриками и процессами можно следить с помощью мониторинга? {#monitoring}

Для всех типов СУБД можно отслеживать:

* загрузку процессора, памяти, сети, дисков в абсолютных величинах;
* загрузку памяти, сети, дисков в процентах от установленных лимитов для класса хостов соответствующего кластера;
* объем данных кластера БД и остаток свободного места в хранилище данных.

Для всех хостов БД можно отслеживать метрики, специфические для типа соответствующей СУБД. Например для {{ GP }} можно отслеживать:

* среднее время выполнения запроса;
* количество запросов в секунду;
* количество ошибок в журналах и т. д.

Мониторинг можно осуществлять с минимальной гранулярностью в 5 секунд.


{% if product == "yandex-cloud" %}

{% include [qa-fz-152.md](../../_includes/qa-fz-152.md) %}

{% endif %}


{% include [qa-logs.md](../../_includes/qa-logs.md) %}

{% include [greenplum-trademark](../../_includes/mdb/mgp/trademark.md) %}
