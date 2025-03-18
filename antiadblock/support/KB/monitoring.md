# Мониторинги

# Содержание
* [Мониторинги cryprox](#cryprox)
    * [Инфраструктурные мониторинги](#cryprox-infra)
    * [Мониторинги квоты YT](#YT-quota)
    * [Мониторинги денег](#cryprox-money)
    * [Мониторинги ошибок](#cryprox-errors)
    * [Другие мониторинги cryprox](#cryprox-others)
* [Мониторинги L7 балансера](#balancer)
    * [Мониторинги ошибок jstracer](#balancer-jstracer)
* [Мониторинги pylon](#pylon)
   * [Мониторинги ошибок и таймингов](#pylon-errors)
* [Другие мониторинги](#other)
* [FAQ](#faq)


## Мониторинги cryprox <a name="cryprox"></a>

`VALUABLE_SERVICES` - [список](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/tasks/tools/const.py?rev=6046527#L1)</br>

### Инфрастуктурые мониторинги <a name="cryprox-infra"></a>
В yp-подах с инстансами cryprox мы следим за тем чтобы
* жесткий диск не был заполнен более чем на 90% (volumes `/logs`, `/perm`, `/`) [Правила уведомлений](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/disk_use).
* потребление cpu было менее 12 (limit контейнера 16) [Правила уведомлений](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/cpu_usage).
* oom killer не убивал процессы в контенере [Правила уведомлений](https://juggler.yandex-team.ru/notification_rules/?query=service%3Dcryprox.oom).

Сообщения о срабатывании приходят в группу [AntiadbInfra](https://t.me/joinchat/Cqt33FQ5Brj30RWzvuGzGA) (но если вас туда еще не добавили, то добавляться не обязательно)

### Мониторинги квоты YT <a name="YT-quota"></a>
Аггрегат Juggler:
```
host=hahn.yt.yandex.net
service=yt_logfeller-antiadb_*
```
CRIT придет, когда останется меньше 10% одной из квот YT: disk_quota, chunk_quota, node_quota, static_quota.

TODO: разработать на этот случай план действий.

### Мониторинги денег <a name="cryprox-money"></a>

Реализованы в виде [скрипта](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/tasks/money_monitoring/__main__.py),
который собирается и загружается как Sandbox-ресурс `ANTIADBLOCK_MONEY_MONITORING_BIN` запускается задачей `ANTIADBLOCK_RUN_BINARY` каждый час и отправляет три вида событий в Juggler:
**aab_report_money_drop_[domain]**</br>
[Код](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/tasks/money_monitoring/data_checks.py#L31)</br>
Резкое падение денег.</br>
Срабатывает если показатели партнера за последние 2-3 часа упали в 0. </br>
**aab_report_no_data_[domain]**</br>
[Код](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/tasks/money_monitoring/data_checks.py#L12)</br>
Отсутствие данных.</br>
Срабатывает если за последние 2-3 часа отсутствуют данные о деньгах партнера.</br>
Сначала следует проверить, доезжают ли логи [если проблема массовая](https://yt.yandex-team.ru/hahn/?page=navigation&path=//home/logfeller/logs/bs-dsp-log/stream/5min), [проблема с  yandex_morda](https://yt.yandex-team.ru/hahn/?page=navigation&path=//logs/awaps-log/stream/5min) и [проблема с morda_direct](https://yt.yandex-team.ru/hahn/?page=navigation&path=//home/logfeller/logs/bs-chevent-log/stream/5min).</br>
**aab_report_negative_trend_[domain]**</br>
[Код](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/tasks/money_monitoring/data_checks.py#L61)</br>
Наличие негативного тренда.</br>
Срабатывает если
* Cкользящее среднее с окном 7 дней упало больше чем на 15%
* Cкользящее среднее с окном 1 день упало больше чем на 30% относительно максимального значения 7-дневного скользящего среднего</br>
**aab_report_update**</br>
[Код](https://a.yandex-team.ru/arc/trunk/arcadia/antiadblock/tasks/money_monitoring/__main__.py#L70)</br>
Есть проблемы доставки данных на стат.</br>
Срабатывает, если [отчет с данными о деньгах](https://stat.yandex-team.ru/AntiAdblock/partners_money_united) давно не обновлялся (примерно 2-3 часа).</br>
Возможные причины:
* TODO: Описать схему подсчета денег

Более подробно про расчеты отклонений в [тикете](https://st.yandex-team.ru/ANTIADB-565).

Сырые события агрегированы per service, алерты приходят в телеграм в чат [ANTIADBMonitoring](https://t.me/joinchat/Cqt33BLwGYNC_Q-3Yh0asQ)</br>
На `VALUABLE_SERVICES` дополнительно есть агрегат со звонком для событий money_drop - [пример агрегата](https://juggler.yandex-team.ru/check_details/?host=antiadb_money_money_drop&service=aab_report_money_drop_service_id_yandex_news_call&last=1DAY)

Когда срабатывает мониторинг, общую картину по партнеру можно посмотреть на Стате [тут](https://stat.yandex-team.ru/AntiAdblock/partners_money_united) и [тут](https://stat.yandex-team.ru/AntiAdblock/partners_money2) если речь о Морде то еще [тут](https://stat.yandex-team.ru/AntiAdblock/morda_awaps_money) и [тут](https://stat.yandex-team.ru/AntiAdblock/morda_actions)
Порядок действий для разных типов событий описан на странице [FAQ](FAQ.md)


### Мониторинги ошибок <a name="cryprox-errors"></a>

### Общее описание
В Соломоне настроены 2 вида алертов</br>
**Алерты с фиксированным порогом (по ДЦ)**</br>
* [4xx](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/errors_4XX_per_dc)
* [5xx](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/errors_5XX_per_dc)

**Разладочные алерты (по service_id)**</br>
_Описание разладок ниже_
* [4xx](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/razladki_4xx_crit)
* [5xx](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/razladki_5xx_crit)
* [rps](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/razladki_rps_crit)

Агрегация этих событий разбита в Джаглере на 4 уровня.</br>
Каждый следующий уровень агрегатов не срабатывает, если сработал предыдущий</br>

**Уровень 1: Проблемы во всех ДЦ**</br>
[Пример агрегата](https://juggler.yandex-team.ru/check_details/?host=antiadb_5xx_errors&service=all_dc_fix_threshold&last=1DAY)</br>
Агрегирует события от алертов с фиксированным порогом с host=cluster.</br>
Оценивается раз в 30 секунд, срабатывает моментально.</br>
Ночью (23:00 - 09:00) навешен 10-минутный флаподав</br>

**Уровень 2: Проблемы в конкретном ДЦ**</br>
[Пример агрегата](https://juggler.yandex-team.ru/check_details/?host=antiadb_5xx_errors&service=sas_fix_threshold&last=1DAY)</br>
Агрегирует события от алертов с фиксированным порогом по ДЦ.</br>
Оценивается раз в 30 секунд, срабатывает с задержкой 2 минуты.</br>
Ночью (23:00 - 09:00) навешен 10-минутный флаподав</br>

**Уровень 3: Проблемы с большим количеством сервисов**</br>
[Пример агрегата](https://juggler.yandex-team.ru/check_details/?host=antiadb_5xx_errors&service=all_services&last=1DAY)</br>
Агрегирует события от разладочных алертов с фиксированным порогом для внутренних сервисов.</br>
Срабатывает, если больше 30% посервисных алертов имеют статус CRIT</br>
Оценивается раз в 30 секунд, срабатывает с задержкой 3 минуты.</br>
Ночью (23:00 - 09:00) навешен 10-минутный флаподав</br>

**Уровень 4: Проблемы с конкретным сервисом**</br>
[Пример агрегата](https://juggler.yandex-team.ru/check_details/?host=antiadb_4xx_errors&service=yandex_mail&last=1DAY)</br>
Агрегирует события от разладочных алертов посервисно.</br>
Оценивается раз в 30 секунд, срабатывает с задержкой в 4 минуты.</br>
На `VALUABLE_SERVICES` дополнительно есть агрегат со звонком, который срабатывает с задержкой в 5 минут от сообщения в Телеграм</br>

#### Описание разладок
**RPS**</br>
[Разладка](https://razladki-wow.n.yandex-team.ru/workspace/256)</br>
1. Загружается линия количества всех запросов с разрешением в 10 сек
2. Линия даунсемплится средним с разрешением в 2 минуты
3. Считаются две переменные has_h_data - наличие точек в последний час, has_m_data - наличие точек в последние 10 минут
4. Вычисляется разладка. Разладкой считается, если больше 80% точек в 20-минутном окне выходит за 4 стандартных отклонения от среднего
5. Среднее рассчитывается Seasonal алгоритмом с разбивкой Будни/Выходные, каждый час считается отдельно
6. Алерт загорается, если has_h_data == true && (has_m_data == false || разладка)

**5XX**</br>
[Разладка](https://razladki-wow.test.n.yandex-team.ru/workspace/64)</br>
1. Загружается линия "отношение количества 5XX /все запросы" за последние 14 дней (разрешение данных: 10 секунд)
2. Разладкой считается, если за последний час больше 50% точек в 10-минутном окне выходит за 3 стандартных отклонения от среднего
3. Среднее рассчитывается Seasonal алгоритмом без разбивки Будни/Выходные, корзинами по 4 часа
4. Считается фиксированный порог - больше 50% точек в 10 минутном окне имеют значение больше 0.05
5. Алерт загорается, если есть разладка или пробит фиксированной порог

**4XX**
[Разладка](https://razladki-wow.test.n.yandex-team.ru/workspace/58)</br>
1. Загружается линия "отношение количества 4XX /все запросы" за последние 14 дней (разрешение данных: 10 секунд)
2. Разладкой считается, если за последний час больше 80% точек в 20-минутном окне выходит за 7 стандартных отклонений от среднего
3. Среднее рассчитывается Seasonal алгоритмом без разбивки Будни/Выходные, корзинами по 4 часа
4. Считается фиксированный порог - больше 80% точек в 20 минутном окне имеют значение больше 0.1
5. Алерт загорается, если есть разладка или пробит фиксированной порог

### Другие мониторинги cryprox <a name="cryprox-others"></a>

#### Мониторинг ошибок jsonify ответа от БК
jsonify - процесс перевода RTB ответа в json.
Мониторинг рабатывает, если в течение 5 минут наблюдается фон больше 20 ошибок в минуту</br>
[Алерт в соломоне](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/Antiadblock_meta_jsonify_errors_alert)</br>
[Агрегат в Джаглере](https://juggler.yandex-team.ru/check_details/?host=antiadb_custom_alerts&service=meta_jsonify_bk_json_parse&last=1DAY)
Алерты приходят в телеграм в чат [ANTIADBMonitoring](https://t.me/joinchat/Cqt33BLwGYNC_Q-3Yh0asQ)</br>


#### Мониторинг на заголовки в рекламных запросах
Основан на 3-х алертах

**Отсутствие заголовков x-real-ip и x-forwarded-for**</br>
Срабатывает, если в течение 5 минут набирается больше 5 рекламных запросов, в которых нет ни одного из заголовков
[Алерт в Соломоне](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/check_headers)

**Алерты на принадлежность заголовков внутренним сетям**</br>
[Алерт x-real-ip](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/check_x_real_ip)</br>
[Алерт x-forwarded-for](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/check_x_forwarded_for)</br>
В десятиминутном окне поминутно считаем уровень (процент) внутренних заголовков</br>
Алерт зажигается, если количество запросов за 10 минут больше 100 и все 10 точек окна больше 5%</br>

Алерты агрегированы по service_id в Джаглере, на каждый алерт - отдельный агрегат</br>
[Пример агрегата](https://juggler.yandex-team.ru/check_details/?host=antiadb_juggler_custom_alerts&service=check_ip_headers_yandex_afisha&last=1DAY)</br>
Уведомления приходят в телеграм-чат [ANTIADBMonitoring](https://t.me/joinchat/Cqt33BLwGYNC_Q-3Yh0asQ)</br>
На `VALUABLE_SERVICES` дополнительно есть агрегат со звонком
[Пример агрегата](https://juggler.yandex-team.ru/check_details/?host=antiadb_juggler_custom_alerts&service=check_ip_headers_yandex_news_call&last=1DAY)</br>
Агрегирован по тому же правилу, но для него настроен флаподав - звонит после 30 минут непрерывных срабатываний

#### Мониторинг на отсутствие кук в рекламных запросах
Проверяет долю рекламных запросов, в которых отсутствуют куки с uid</br>
[Алерт в Соломоне](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/check_uid_cookies)</br>
Настроен через [разладку](https://razladki-wow.n.yandex-team.ru/workspace/656)</br>
Описание разладки:</br>
Каждую минуту (так работают все алерты в Соломоне)
1. Загружается линия "отношение запросы без кук/все запросы" за последние 14 дней (разрешение данных: 10 секунд)
2. Линия даунсемплится средним с разрешением в 1 минуту
3. Вычисляется переменная фиксированный порог - в 10-минутном окне все точки больше 5%.
4. Вычисляется разладка - в 10-минутном окне больше 80% точек отклоняются от среднего больше, чем на 3 стандартных отклонения
5. Алерт загорается, если горят оба пункта 3 и 4
6. Фиксированный порог нужен для того, чтобы не зажигать алерты на сервисы, у которых очень мало запросов без кук

[Пример агрегата](https://juggler.yandex-team.ru/check_details/?host=antiadb_juggler_custom_alerts&service=check_uid_cookies_yandex_realty&last=1DAY)</br>
Уведомления приходят в телеграм-чат [ANTIADBMonitoring](https://t.me/joinchat/Cqt33BLwGYNC_Q-3Yh0asQ)</br>


#### Мониторинг событий AWAPS на Морде
Настроен на основе 2х алертов</br>
Описание разладки:</br>
Каждую минуту (так работают все алерты в Соломоне)
1. Загружается отношение линий показы за адблоком/розыгрыши за адблоком (разрешение данных: 10 минут)
2. Загружается отношение линий общие показы/общие розыгрыши (разрешение данных: 10 минут)
3. Для каждой линии разладкой считается, если за последний час больше 2/3 точек выходит за 3 стандартных отклонения от среднего
4. Среднее рассчитывается Seasonal алгоритмом с разбивкой Будни/Выходные, каждый час считается отдельно
5. Алерт загорается, если есть разладка в заадблочной линии, но нет в линии с общими событиями

__Отношение показы/розыгрыши в заадблочной схеме__
[Алерт в Соломоне](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/morda_relative_actions)</br>
Настроен через [разладку](https://razladki-wow.n.yandex-team.ru/workspace/431)</br>


__Отношение количества заадблочных событий к общему количеству__
[Алерт в Солмоне](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/morda_actions_by_action)</br>
Настроен через [разладку](https://razladki-wow.n.yandex-team.ru/workspace/430)</br>
Параметризован по событиям 0 (показ), 15 (розыгрыш), 95 (загрузка кода баннера)</br>
Описание разладки:</br>
Каждую минуту (так работают все алерты в Соломоне)
1. Загружается отношение линий количество адблочных событий/общее количество событий (разрешение данных: 10 минут)
2. Алерт загорается, если за последний час больше 2/3 точек выходит за 3 стандартных отклонения от среднего
3. Среднее рассчитывается Seasonal алгоритмом с разбивкой Будни/Выходные, каждый час считается отдельно

Алерты агрегированы в Джаглере по правилу `ИЛИ`
На агрегате настроен флаподав - уведомление придет в случае 15 минут непрерывных проблем.</br>
Флаподав нужен, так как статус, описанных выше алертов моментально отображается на [дашборде](https://antiblock.yandex.ru/service/yandex_morda/health)</br>
[Агрегат в Джаглере](https://juggler.yandex-team.ru/check_details/?host=antiadb_juggler_custom_alerts&service=morda_actions&last=1DAY)</br>
Уведомления приходят в телеграм-чат [ANTIADBMonitoring](https://t.me/joinchat/Cqt33BLwGYNC_Q-3Yh0asQ)</br>

#### Мониторинг событий bamboozled
[Алерт в Соломоне](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/bamboozled_crit)
Настроен через [разладку](https://razladki-wow.n.yandex-team.ru/workspace/657)
Описание разладки:</br>
1. Загружаются 2 линии: количество try_to_render, количество confirm_block
2. Сглаживаются оконнным сглаживанием с окном в 5 минут
3. Для отношения confirm_block/try_to_render разладкой считается, если все точки 10-минутного окна отклоняются на 5 стандартных отклонений от среднего
4. Среднее рассчитывается скользящим средним, для обучения берутся только последние сутки от всей линии. Такие параметры используются, чтобы ловить резкие изменения.
5. Для линии confirm_block разладкой считается, если все точки 30-минутного окна отклоняются на 5 стандартных отклонений от среднего
6. Среднее рассчитывается Seasonal алгоритмом с разбивкой Будни/Выходные, каждые 2 часа считаются отдельно

#### Мониторинг уровня фрода
Данные по фроду в Соломоне разбиты на 6 сенсоров в зависимости от источника (bad - "быстрый" фрод движка, fraud - "медленный фрод по логам") и типа фрода - показы, клики, деньги</br>
[Алерт в Соломоне](https://solomon.yandex-team.ru/admin/projects/Antiadblock/alerts/fraud) сгруппирован по каждому из сенсоров и настроен через [разладку](https://razladki-wow.n.yandex-team.ru/workspace/659)</br>
Описание разладки:</br>
1. Загружается линия 10-минутных данных за последние 14 дней. Данные загружаются с задержкой в 2 часа - это соответствует задержке появления свежих данных в Solomon.
2. Среднее рассчитывается сезонным алгоритмом с количеством бакетов 24, от линии отбрасываются последние 24 часа
3. Разладкой считается отклонение более 90% 10-минутных точек от среднего более чем на 6 стандартных отклонений в 30-минутном окне (только вверх, нижнего порога нет)
4. Также рассчитывается отклонение точек от фикисрованного порога 2%
5. Алерт срабатывает если (есть разладка И точки выше фиксированного порога в 2%) ИЛИ нет 10-минутных данных за последний час

В Джаглере на каждую пару (источник фрода, тип фрода) настроено отдельное уведомление.</br>
[Пример агрегата](https://juggler.yandex-team.ru/check_details/?host=antiadb_fraud&service=fraud_autoru_money&last=1DAY)


## Мониторинги L7 балансера <a name="balancer"></a>

### Мониторинги ошибок jstracer <a name="balancer-jstracer"></a>
Сообщения приходят в чатик [AntiadbInfra](https://t.me/joinchat/DUh11VQ5Brh7BDEOk7SOQg)   
О проблемах нужно сообщить в телеграмм-чат [JS Tracer](https://t.me/joinchat/Bx_fClIKobJed5KxGO6WEQ) (наш дежурный аккаунт в этом чате есть)    
Во всех мониторингах меряется средний процент событий от общего количества запросов в 3-х минутном окне    
Границы срабатываний можно посмотреть и настроить в UI:
* [Мониторинг 5xx](https://yasm.yandex-team.ru/alert/cryprox-jstracer-5xx)  
Границы срабатываний: WARN 3-6%, CRIT >6%
* [Мониторинг 4xx](https://yasm.yandex-team.ru/alert/cryprox-jstracer-4xx)  
Границы срабатываний: WARN 3-6%, CRIT >6%
* [Мониторинг backend-errors](https://yasm.yandex-team.ru/alert/cryprox-jstracer-backend-errors)  
Измеряет количество событий connection-timeout и backend-timeout, границы срабатываний: WARN 4-8%, CRIT >8%


## Мониторинги pylon<a name="pylon"></a>

### Мониторинги ошибок и таймингов <a name="pylon-errors"></a>

#### Мониторинг таймингов
Срабатывает, если хотя бы на одной машине, 98 перцентиль таймингов больше 5 секунд и держится дольше 5 минут</br>
Алерты приходят в телеграм в чат ANTIADBMonitoring</br>

Агрегирован в джаглере по партнерам (то есть оповещения приходят отдельно по каждому партнеру):
* [Эхо Москвы (pylon.yandex.net:pylon_percentile_echo)](https://juggler.yandex-team.ru/check_details/?service=pylon_percentile_echo&host=pylon.yandex.net&last=1DAY)
* [Недвижимость (pylon.yandex.net:pylon_percentile_realty)](https://juggler.yandex-team.ru/check_details/?service=pylon_percentile_realty&host=pylon.yandex.net&last=1DAY)

#### Мониторинг ошибок
Срабатывает, если в течение 5 минут в любом из ДЦ процент 5XX ошибок больше 10%</br>

Агрегат в джаглере:
* [pylon_errors](https://juggler.yandex-team.ru/check_details/?service=pylon_errors&host=antiadb_pylon&last=1DAY)


## Другие мониторинги <a name="other"></a>

### NOC
Жора подписан на рассылку @noc-announces
В телеграме так же есть дежурный аккаунт ноков (http://t.me/NocDuty)
Если есть подозрения на сеть/ДЦ итд можно писать сразу ему

## FAQ <a name="faq"></a>
__Сработал мониторинг. Где посмотреть подробности?__</br>
Идем по ссылке из сообщения в [Соломон](https://solomon.yandex-team.ru)</br>
Жмем `Show chart`
![screenshot](https://jing.yandex-team.ru/files/ddemidov/Screen_Shot_2018-07-24_at_16_17_23.png)
