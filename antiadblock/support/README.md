# Дежурство и поддержка партнеров в Антиадблоке

[График дежурств](https://abc.yandex-team.ru/services/antiadblock/duty/) </br>

На этой странице собрана основная информация, которая может понадобится во время дежурства и подключения партнеров.

**Основная задача дежурного - сделать так, чтобы боевые товарищи не отвлекались от разработки мега-фичей и фикса неуловимых багов
на мелкие вопросы вроде падения денег и вопросы партнеров**

## Перед началом дежурства

1. Посмотреть открытые тикеты в очереди [ANTIADBSUP](https://st.yandex-team.ru/filters/filter:25387)
В каждом тикете есть информация о том, в каком статусе он находится.  
2. Если какой-то информации нет или непонятно, что происходит с каким-то тикетом, спроси у предыдущего дежурного.
Кто предыдущий дежурный? - См. [График](https://wiki.yandex-team.ru/BannernajaKrutilka/duty/DutyPlan/)
3. Залогиниться под сапортным аккаунтом в [телеграме](KB/accounts.md#Жора-Антиадблоков), [почте](KB/accounts.md#Робот-на-стаффе) и [YAMB'е](KB/accounts.md#Робот-на-стаффе)
4. Запросить необходимые [роли](KB/accounts.md#Роли)
5. Прочитать [регламент](KB/regulations.md)
6. Настроить себе СМС-оповещение [мониторинги](KB/monitoring.md)

## Во время дежурства

### Куда смотреть?
1. [Мониторинги](KB/monitoring.md)
2. Чаты партнеров 
3. [Партнерские графики](https://solomon.yandex-team.ru/?project=Antiadblock&cluster=cryprox-prod&service=cryprox_actions&dashboard=Antiadblock_partner_dashboard&l.service_id=autoru)
4. [График с деньгами](https://stat.yandex-team.ru/AntiAdblock/partners_money)

### Как работать с триггерами?

0. Сработал триггер: Упали деньги | Написал партнер | Сработал мониторинг
1. Если это сигнал от партнера - сразу ответить, что разбираемся
2. Если за 10-15 минут разобраться не удалось,  завести тикет в [ANTIADBSUP](https://st.yandex-team.ru/ANTIADBSUP/)
3. Написать партнеру краткое резюме проблемы
4. Разбираться 
5. Сообщать партнерам о статусе минимум раз в день

### Заведение тикетов

__Если сработал любой из мониторингов - должен быть тикет!!!__

[Как завести тикет](KB/tickets.md)


## Перед окончанием дежурства

1. Пройтись по всем тикетам, обновить статусы
2. Перенести все кейсы в KB через ПР в этот репозиторий

## Полезные ссылки для дежурного (графики, дашборды, логи etc)

### Прокси
* [Логи прокси в эластике](https://ubergrep.yandex-team.ru)
* [Документация по проксе для партнеров](https://tech.yandex.ru/antiadblock/doc/concepts/about-docpage/)
* [График Nginx Errors](https://ubergrep.yandex-team.ru/goto/a0805a2499a350529d2ef41cf2f18e4e)

### Морда и авапс
* [Главный дашборд Авапс](https://grafana.yandex-team.ru/dashboard/db/awaps-main?refresh=1m&orgId=1)
* [Статовский отчет авапса по деньгам](https://stat.yandex-team.ru/DisplayAds/reports/prodtype?scale=d&adtype=_in_table_&_incl_fields=budget&_incl_fields=impression&_incl_fields=cpm&_incl_fields=click&_incl_fields=cpc&_incl_fields=ctr&_incl_fields=flight&date_min=2018-04-01+00%3A00%3A00&date_max=2018-04-18+23%3A59%3A59)
* [Таблица о событиях на Морде (Стат)](https://stat.yandex-team.ru/AntiAdblock/morda_actions?scale=i&actionid=_in_table_&_incl_fields=aab_count&_incl_fields=total_count&_period_distance=200).
* [Wiki-страница с релизами Морды](https://wiki.yandex-team.ru/morda/release/)

### Остальные партнеры
* [Тикеты на подключение партнеров](https://st.yandex-team.ru/filters/filter:25201)
* [Партнерский дашборд в Соломоне](https://nda.ya.ru/3UWckV)
* [Партнеры со статусами подключения](https://wiki.yandex-team.ru/ANTIADB/partners/)
* [Менеджеры партнеров в РСЯ](https://wiki.yandex-team.ru/antiadb/Menedzhery-partnjorov/)
* [Дашборд по партнерам](https://grafana.yandex-team.ru/dashboard/db/antiadb-partners?refresh=1m&orgId=1)
* [Графики о деньгах партнеров (Графана)](https://grafana.yandex-team.ru/dashboard/db/antiadb-money-with-unblock?refresh=1h&orgId=1)
* [Графики о деньгах партнеров (Стат)](https://stat.yandex-team.ru/Dashboard/products/AntiAdblock/AABMoneyPercent)
* [Таблица о деньгах партнеров (Стат)](https://stat.yandex-team.ru/-/CBeefPtRwD) 
* [Правила Адблока, действующие на партнеров](https://yt.yandex-team.ru/hahn/#page=navigation&path=//home/antiadb/sonar/sonar_rules)
* [Проверить корректность кукиматчинга](https://wiki.yandex-team.ru/antiadb/Proverka-partnerov/)
* [Информация по внутренним партнерам в РСЯ](https://st.yandex-team.ru/YANPARTNERS-348)

### Адблок
* [Синтаксис фильтров для Adblock Plus](https://adblockplus.org/filters)
* [Синтаксис фильтров для Adguard](https://kb.adguard.com/ru/general/how-to-create-your-own-ad-filters)
* [Ruadlist repo](https://hg.adblockplus.org/ruadlist)

### Wiki/FAQ/Другая документация
* [Подключение партнеров](KB/partners.md)
* [Куки-матчинг](KB/cookie-matching.md)
* [FAQ](KB/FAQ.md)
* [Документация Адфокса](https://sites.help.adfox.ru/)
* [Мониторинг правил](https://github.yandex-team.ru/AntiADB/adblock-rule-sonar/blob/master/README.md)
* [Тестовая страница медийных кампаний Директа](http://fcmb.ru/reach_product.html)
* [График дежурств БК (в том числе партнерский код)](https://wiki.yandex-team.ru/bannernajakrutilka/duty/dutyplan/)
