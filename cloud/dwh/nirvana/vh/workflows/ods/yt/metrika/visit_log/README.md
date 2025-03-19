#### Visits

Таблица визитов на сервисы Яндекса, содержащая информацию только по следующим счетчикам:
- 50027884, 48570998, 51465824
Таблица сформирована на базе [RAW](../../../../raw/yt/statbox/visit_log/README.md).

##### Схема

[PROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/prod/ods/metrika/visit_log)
/ [PREPROD](https://yt.yandex-team.ru/hahn/navigation?path=//home/cloud-dwh/data/preprod/ods/metrika/visit_log)

- `browser_country`                         - Страна, установленная в браузере
- `browser_engine_id`                       - Идентификатор движка браузера
- `browser_engine_version1`                 - Версия движка браузера
- `browser_engine_version2`                 - Версия движка браузера
- `browser_engine_version3`                 - Версия движка браузера
- `browser_engine_version4`                 - Версия движка браузера
- `browser_language`                        - Язык, установленный в браузере
- `counter_id`                              - id счетчика
- `crypta_id`                               - CryptaID v2
- `device_model`                            - DeviceModel из uatraits
- `device_pixel_ratio`                      - Пиксельное соотношение
- `domain_zone`                             - Домен куки yandexuid
- `duration`                                - Длительность визита в секундах
- `end_url`                                 - Урл, на котором закончился визит
- `event_id`                                - Хиты, которые были в это визите, кроме параметров визитов. Ограничение массива - 500 хитов. Аналог поля WatchIDs
- `event_involved_time`                     - Время, в течение которого пользователь видел страницу
- `event_is_page_view`                      - Является ли событие просмотром
- `FirstVisit`                              - Время и дата первого визита посетителя
- `goal_reaches_any`                        - Количество достижений целей
- `goals_event_time`                        - Время достижения каждой цели
- `goals_id`                                - Идентификатор целей, достигнутых за этот визит
- `hits`                                    - Число просмотров в визите 
- `is_bounce`                               - Является ли визит отказом
- `is_download`                             - Была ли загрузка в этом визите
- `is_logged_in`                            - Был ли посетитель залогинен на Яндексе
- `is_mobile`                               - Хит был с десктопного или мобильного браузера
- `is_private_mode`                         - Приватный режим браузера
- `is_robot`                                - Роботность визита
- `is_tv`                                   - Визит был с планшета или нет
- `is_tablet`                               - Был ли хит из Турбо страницы
- `is_turbo_page`                           - Визит был с телевизора или нет
- `is_web_view`                             - Был ли хит из WebView
- `last_visit`                              - Дата последнего визита посетителя
- `link_url`                                - Урл внешней ссылки, если на ней был переход
- `mobile_phone_model`                      - Полное название модели мобильного телефона
- `mobile_phone_vendor`                     - Идентификатор мобильного телефона
- `os`                                      - Идентификатор ОС
- `os_family`                               - OSFamily из uatraits
- `os_name`                                 - OSName из uatraits
- `parsed_params_key_1`                     - Параметр, уровень 1
- `parsed_params_key_10`                    - Параметр, уровень 10
- `parsed_params_key_2`                     - Параметр, уровень 2
- `parsed_params_key_3`                     - Параметр, уровень 3
- `parsed_params_key_4`                     - Параметр, уровень 4
- `parsed_params_key_5`                     - Параметр, уровень 5
- `parsed_params_key_6`                     - Параметр, уровень 6
- `parsed_params_key_7`                     - Параметр, уровень 7
- `parsed_params_key_8`                     - Параметр, уровень 8
- `parsed_params_key_9`                     - Параметр, уровень 9
- `puid`                                    - id пользователя (либо puid, либо сгенерированный на базе ip и User-Agent)*
- `publisher_events_adv_engine_id`          - Источник трафика для статьи - рекламная система
- `publisher_events_article_height`         - Высота статьи в пикселях
- `publisher_events_article_id`             - Идентификатор статьи
- `publisher_events_authors`                - Авторы статьи
- `publisher_events_chars`                  - Число знаков в тексте статьи
- `publisher_events_event_id`               - WatchID просмотра, к которому относится статья
- `publisher_events_from_article_id`        - Идентификатор статьи, с которой перешли на текущую
- `publisher_events_has_recircled`          - Была ли рециркуляция с просмотра (перешел ли пользователь на другую статью на сайте)
- `publisher_events_hit_event_time`         - Время хита
- `publisher_events_involved_time`          - Время, в течение которого пользователь видел статью
- `publisher_events_messenger_id`           - Источник трафика для статьи - мессенджер
- `publisher_events_publication_time`       - Дата публикации статьи
- `publisher_events_recommendation_syste`   - Источник трафика для статьи - рекомендательная система
- `publisher_events_referrer_domain`        - Источник трафика для статьи - домен реферерра
- `publisher_events_referrer_path`          - Источник трафика для статьи - путь реферерра
- `publisher_events_rubric`                 - Рубрика статьи
- `publisher_events_rubric_2`               - Рубрика второго уровня для статьи
- `publisher_events_scroll_down`            - Показывает, насколько была проскроллена статья: -1 - не была показана, 0 - видел начало, 1 - прокроллил на 1/3, 2 - проскроллил на 2/3, 3 - проскроллил до конца, 4 - проскроллил дальше (хотя бы на полэкрана после конца статьи).
- `publisher_events_search_engine_id`       - Источник трафика для статьи - поисковая система
- `publisher_events_social_source_networ`   - Источник трафика для статьи - социальная сеть
- `publisher_events_title`                  - Заголовок статьи
- `publisher_events_topics`                 - Тематики статьи
- `publisher_events_trafic_source`          - Источник трафика для статьи
- `publisher_events_turbo_type`             - Тип турбо для статьи: 0 - не Турбо, 1 - Яндекс, 2 - Google, 3 - Facebook
- `publisher_events_url_canonical`          - Каноничный URL статьи
- `referer`                                 - Реферер
- `referer_domain`                          - Домен реферера
- `region_id`                               - Идентификатор региона посетителя
- `resolution_depth`                        - Глубина цвета
- `resolution_height`                       - Высота экрана
- `resolution_width`                        - Ширина экрана
- `search_engine_id`                        - Идентификатор поисковой системы
- `search_phrase`                           - Поисковый запрос
- `social_source_network_id`                - Идентификатор социальной сети с которой был переход
- `start_date`                              - Дата начала визита
- `start_url`                               - URL с которого начался визит
- `start_url_domain`                        - Домен стартового урла
- `top_level_domain`                        - Домен верхнего уровня
- `total_visits`                            - Общее количество визитов посетителя
- `trafic_source_id`                        - Детальное описание возможных значений можно найти [здесь](https://wiki.yandex-team.ru/jandexmetrika/data/metrikatables/visits/traficsourceid/?from=%2Fjandexmetrika%2Fdoc%2Fvisordfd%2Fmtlog%2Fvisits%2FTraficSourceID%2F)
- `event_start_dt_utc`                      - Дата и время начала визита в UTC
- `utm_campaign`                            - Название рекламной кампании UTM (Вырезается из URL первого просмотра)
- `utm_content`                             - Дополнительная информация по UTM (Вырезается из URL первого просмотра)
- `utm_medium`                              - Тип рекламы UTM (Вырезается из URL первого просмотра)
- `utm_source`                              - Имя UTM метки, рекламная площадка (Вырезается из URL первого просмотра)
- `utm_term`                                - Ключевая фраза UTM (Вырезается из URL первого просмотра)
- `user_agent`                              - Идентификатор браузера
- `user_agent_major`                        - Первое число версии браузера
- `user_agent_minor`                        - Второе число версии браузера
- `user_agent_version_2`                    - Полная версия браузера, число 2
- `user_agent_version_3`                    - Полная версия браузера, число 3
- `user_agent_version_4`                    - Полная версия браузера, число 4
- `user_id`                                 - Идентификатор юзера
- `user_id_type`                            - Тип UserID
- `visit_id`                                - id визита
- `window_client_height`                    - Высота окна клиента
- `window_client_width`                     - Ширина окна клиента
- `yandex_login`                            - Логин на Яндексе

_Note_:
* puid отсутвует в следующих случаях:
- Режим Incognito в браузере
- Использование плагинов типа AdBlock
- Активность робота
- Intelligent Tracking Prevention в некоторых броузерах
