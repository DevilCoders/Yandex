# coding: utf-8
from antiadblock.tasks.money_tickets_updater.plugins.awaps_morda import get_awaps_info
from antiadblock.tasks.money_tickets_updater.plugins.bamboozled import get_bamboozled_info
from antiadblock.tasks.money_tickets_updater.plugins.bypass_experiment import get_bypass_experiment_info
from antiadblock.tasks.money_tickets_updater.plugins.experiments import get_experiments_info
from antiadblock.tasks.money_tickets_updater.plugins.fraud import get_fraud_info
from antiadblock.tasks.money_tickets_updater.plugins.run_argus import get_argus_info
from antiadblock.tasks.money_tickets_updater.plugins.statistics_by_partner_pageid import get_statistics_by_partner_pageid
from antiadblock.tasks.money_tickets_updater.plugins.unblock import get_unblock_info
from antiadblock.tasks.money_tickets_updater.plugins.detect import get_detect_checker_info
from antiadblock.tasks.money_tickets_updater.plugins.log_action import get_log_action_info


plugins = {
    'Правила': lambda service_id, date_start: 'https://antiblock.yandex.ru/service/{}/rules'.format(service_id),
    'Bamboozled': get_bamboozled_info,
    'Ложный детект (результаты bypass эксперимента)': get_bypass_experiment_info,
    'Результат проверки детекта': get_detect_checker_info,
    'Процент фрода': get_fraud_info,
    'История изменения конфигов': get_log_action_info,
    'Ссылка на запуск в Аргус': get_argus_info,
    'Процент разблока': get_unblock_info,
    'Эксперименты и релизы партнера': get_experiments_info,
    'Статистика в разрезе по партнерским PageId': get_statistics_by_partner_pageid,
    'Статистика по Авапсу на Морде': get_awaps_info,
}
