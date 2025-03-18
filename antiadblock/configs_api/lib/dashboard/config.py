# coding=utf-8
from .yql_queries import WEEK_RULES_QUERY, PCODE_VERSIONS_QUERY

DASHBOARD_API_CONFIG = dict(
    groups=[
        dict(
            group_id=u'errors',
            group_title=u'Ошибки',
            group_ttl=3600,
            group_update_period=60,
            checks=[
                dict(
                    check_type=u'solomon',
                    check_id=u'rps_errors',
                    check_title=u'RPS',
                    check_args=dict(
                        alert_id=u'razladki_antiadb_rps',
                    ),
                    description=u'Irregular RPS value',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'4xx_errors',
                    check_title=u'4xx Ошибки',
                    check_args=dict(
                        alert_id=u'razladki_antiadb_4xx_errors',
                    ),
                    description=u'Irregular 4xx percent',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'5xx_errors',
                    check_args=dict(
                        alert_id=u'razladki_antiadb_5xx_errors',
                    ),
                    check_title=u'5xx Ошибки',
                    description=u'Irregular 5xx percent',
                ),
            ]
        ),
        dict(
            group_id=u'bamboozled_by_app',
            group_title=u'Разблок по блокировщикам',
            group_ttl=3600,
            group_update_period=60,
            checks=[
                dict(
                    check_type=u'solomon',
                    check_id=u'bamboozled_adblock',
                    check_title=u'ADBLOCK',
                    check_args=dict(
                        alert_id=u'bamboozled_by_app',
                        alert_labels=dict(app=u"ADBLOCK")
                    ),
                    description=u'Разблок в ADBLOCK',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'bamboozled_adguard',
                    check_title=u'ADGUARD',
                    check_args=dict(
                        alert_id=u'bamboozled_by_app',
                        alert_labels=dict(app=u"ADGUARD")
                    ),
                    description=u'Разблок в ADGUARD',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'bamboozled_ublock',
                    check_title=u'UBLOCK',
                    check_args=dict(
                        alert_id=u'bamboozled_by_app',
                        alert_labels=dict(app=u"UBLOCK")
                    ),
                    description=u'Разблок в UBLOCK',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'bamboozled_unknown',
                    check_title=u'UNKNOWN',
                    check_args=dict(
                        alert_id=u'bamboozled_by_app',
                        alert_labels=dict(app=u"UNKNOWN")
                    ),
                    description=u'Разблок в UNKNOWN',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'bamboozled_not_blocked',
                    check_title=u'NOT_BLOCKED',
                    check_args=dict(
                        alert_id=u'bamboozled_by_app',
                        alert_labels=dict(app=u"NOT_BLOCKED")
                    ),
                    description=u'Разблок в NOT_BLOCKED',
                )
            ]
        ),
        dict(
            group_id=u'bamboozled_by_browser',
            group_title=u'Разблок по браузерам',
            group_ttl=3600,
            group_update_period=60,
            checks=[
                dict(
                    check_type=u'solomon',
                    check_id=u'bamboozled_chrome',
                    check_title=u'Chrome',
                    check_args=dict(
                        alert_id=u'bamboozled_by_browser',
                        alert_labels=dict(browser=u"chrome")
                    ),
                    description=u'Разблок в Chrome',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'bamboozled_edge',
                    check_title=u'Edge',
                    check_args=dict(
                        alert_id=u'bamboozled_by_browser',
                        alert_labels=dict(browser=u"edge")
                    ),
                    description=u'Разблок в Edge',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'bamboozled_safari',
                    check_title=u'Safari',
                    check_args=dict(
                        alert_id=u'bamboozled_by_browser',
                        alert_labels=dict(browser=u"safari")
                    ),
                    description=u'Разблок в Safari',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'bamboozled_firefox',
                    check_title=u'Firefox',
                    check_args=dict(
                        alert_id=u'bamboozled_by_browser',
                        alert_labels=dict(browser=u"firefox")
                    ),
                    description=u'Разблок в Firefox',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'bamboozled_opera',
                    check_title=u'Opera',
                    check_args=dict(
                        alert_id=u'bamboozled_by_browser',
                        alert_labels=dict(browser=u"opera")
                    ),
                    description=u'Разблок в Opera',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'bamboozled_yabro',
                    check_title=u'Yandex Browser',
                    check_args=dict(
                        alert_id=u'bamboozled_by_browser',
                        alert_labels=dict(browser=u"yandex_browser")
                    ),
                    description=u'Разблок в Yandex Browser',
                )
            ]
        ),
        #
        # dict(
        #     group_id=u'Rules',
        #     group_title=u'Новые правила',
        #     group_ttl=3600,
        #     group_update_period=3600,
        #     checks=[
        #         dict(
        #             check_type=u'yql',
        #             check_id=u'last_week_rules',
        #             check_title=u'Правила за неделю',
        #             check_args=dict(
        #                 query=WEEK_RULES_QUERY
        #             ),
        #             description=u'Правила вышедшие за последние 6 и 7 дней',
        #         )
        #     ]
        # ),
        dict(
            group_id=u'Detect',
            group_title=u'Результат проверки детекта',
            group_ttl=14400,
            group_update_period=180,
            checks=[
                dict(
                    check_type=u'stat',
                    check_id=u'detect_adblockplus_chrome',
                    check_title=u'Chrome/Adblock Plus',
                    check_args=dict(
                        browser=u'chrome',
                        blocker=u'AdblockPlus',
                        report=u'AntiAdblock/detect-checker-results'
                    ),
                    description=u'Проверка детекта Chrome Adblock Plus',
                ),
                dict(
                    check_type=u'stat',
                    check_id=u'detect_adblockplus_firefox',
                    check_title=u'Firefox/Adblock Plus',
                    check_args=dict(
                        browser=u'firefox',
                        blocker=u'AdblockPlus',
                        report=u'AntiAdblock/detect-checker-results'
                    ),
                    description=u'Проверка детекта Firefox Adblock Plus',
                ),
                dict(
                    check_type=u'stat',
                    check_id=u'detect_adguard_firefox',
                    check_title=u'Firefox/Adguard',
                    check_args=dict(
                        browser=u'firefox',
                        blocker=u'Adguard',
                        report=u'AntiAdblock/detect-checker-results'
                    ),
                    description=u'Проверка детекта Firefox Adguard',
                ),
                dict(
                    check_type=u'stat',
                    check_id=u'detect_adguard_chrome',
                    check_title=u'Chrome/Adguard',
                    check_args=dict(
                        browser=u'chrome',
                        blocker=u'Adguard',
                        report=u'AntiAdblock/detect-checker-results'
                    ),
                    description=u'Проверка детекта Chrome Adguard',
                ),
                dict(
                    check_type=u'stat',
                    check_id=u'detect_ublock_origin_chrome',
                    check_title=u'Chrome/Ublock Origin',
                    check_args=dict(
                        browser=u'chrome',
                        blocker=u'Ublock Origin',
                        report=u'AntiAdblock/detect-checker-results'
                    ),
                    description=u'Проверка детекта Chrome Ublock Origin',
                ),
                dict(
                    check_type=u'stat',
                    check_id=u'detect_ublock_origin_firefox',
                    check_title=u'Firefox/Ublock Origin',
                    check_args=dict(
                        browser=u'firefox',
                        blocker=u'Ublock Origin',
                        report=u'AntiAdblock/detect-checker-results'
                    ),
                    description=u'Проверка детекта Firefox Ublock Origin',
                ),
                dict(
                    check_type=u'stat',
                    check_id=u'detect_without_adblock_chrome',
                    check_title=u'Chrome/Without Adblock',
                    check_args=dict(
                        browser=u'chrome',
                        blocker=u'Without Adblock',
                        report=u'AntiAdblock/detect-checker-results'
                    ),
                    description=u'Проверка детекта Chrome Without Adblock',
                ),
                dict(
                    check_type=u'stat',
                    check_id=u'detect_without_adblock_firefox',
                    check_title=u'Firefox/Without Adblock',
                    check_args=dict(
                        browser=u'firefox',
                        blocker=u'Without Adblock',
                        report=u'AntiAdblock/detect-checker-results'
                    ),
                    description=u'Проверка детекта Firefox Without Adblock',
                ),
            ]
        ),
        dict(
            group_id=u'unblock',
            group_title=u'Процент разблока по продуктам',
            group_ttl=3600,
            group_update_period=60,
            checks=[
                dict(
                    check_type=u'solomon',
                    check_id=u'unblock_all_products',
                    check_title=u'Все продукты',
                    check_args=dict(
                        alert_id=u'unblock_daily',
                        alert_labels=dict(producttype=u'_all')
                    ),
                    description=u'Процент разблока по всем продуктам',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'unblock_direct',
                    check_title=u'Директ',
                    check_args=dict(
                        alert_id=u'unblock_daily',
                        alert_labels=dict(producttype=u'direct')
                    ),
                    description=u'Процент разблока Директ',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'unblock_media_creative',
                    check_title=u'Медийка',
                    check_args=dict(
                        alert_id=u'unblock_daily',
                        alert_labels=dict(producttype=u'media-creative')
                    ),
                    description=u'Процент разблока медийки',
                )
            ]
        ),
        dict(
            group_id=u'morda_actions',
            group_title=u'События на Морде',
            group_ttl=3600,
            group_update_period=60,
            checks=[
                dict(
                    check_type=u'solomon',
                    check_id=u'relative_actions',
                    check_title=u'Показы/Розыгрыши',
                    check_args=dict(
                        alert_id=u'morda_relative_actions',
                    ),
                    description=u'Отношение розыгрышей к показам в антиадблочной схеме',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'action_0',
                    check_title=u'Показы(0)',
                    check_args=dict(
                        alert_id=u'morda_actions_by_action',
                        alert_labels=dict(action=u'0')
                    ),
                    description=u'Отношение количества показов в антиадблочной схеме к обычным',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'action_15',
                    check_title=u'Розыгрыши(15)',
                    check_args=dict(
                        alert_id=u'morda_actions_by_action',
                        alert_labels=dict(action=u'15')
                    ),
                    description=u'Отношение количества розыгрышей в антиадблочной схеме к обычным',
                ),
                dict(
                    check_type=u'solomon',
                    check_id=u'action_95',
                    check_title=u'Загрузки кода баннеров(95)',
                    check_args=dict(
                        alert_id=u'morda_actions_by_action',
                        alert_labels=dict(action=u'95')
                    ),
                    description=u'Отношение количества загрузок кода баннеров в антиадблочной схеме к обычным',
                )
            ]
        ),
        # dict(
        #     group_id=u'pcode_versions',
        #     group_title=u'Разблок по версиям PCODE',
        #     group_ttl=3600,
        #     group_update_period=3600,
        #     checks=[
        #         dict(
        #             check_type=u'yql',
        #             check_id=u'top_{}_pcode_version'.format(position),
        #             check_title=u'Version #{}'.format(position),
        #             check_args=dict(
        #                 query=PCODE_VERSIONS_QUERY,
        #                 placeholders=dict(
        #                     _version_position_=position
        #                 )
        #             ),
        #             description=u'Разблок по ТОП-5 версиям ПКОДа',
        #         ) for position in range(1, 6)
        #     ]
        # ),
        dict(
            group_id=u'fraud',
            group_title=u'Фрод',
            group_ttl=3600,
            group_update_period=60,
            checks=[
                dict(
                    check_type=u'solomon',
                    check_id=u'fraud_{}'.format(fraud_type),
                    check_title=u'{}'.format(fraud_title),
                    check_args=dict(
                        alert_id=u'fraud',
                        alert_labels=dict(sensor=fraud_type)
                    ),
                    description=u'Уровень фрода ({})'.format(fraud_title)
                ) for fraud_type, fraud_title in {
                    u'bad_money_percent': u'Деньги (event_bad)',
                    u'bad_clicks_percent': u'Клики (event_bad)',
                    u'bad_shows_percent': u'Показы (event_bad)',
                    u'fraud_money_percent': u'Деньги (chevent_fraud)',
                    u'fraud_clicks_percent': u'Клики (chevent_fraud)',
                    u'fraud_shows_percent': u'Показы (chevent_fraud)',
                }.iteritems()]
        )
    ]
)
