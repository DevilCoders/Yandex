#!/usr/bin/env python
# encoding=utf8
import argparse
import logging
from os import getenv
from smtplib import SMTP

import yt.wrapper as yt

import antiadblock.analytics_service.service.modules as modules

# constants
CLUSTER = 'hahn'
RESULT_TABLE = '//home/antiadb/analytics'

MODULES = modules.__all__


def config_yt_client(token):
    yt.config['proxy']['url'] = CLUSTER
    yt.config['token'] = token


def get_previous_result():
    # сразу переводим в словарь {ключ: [версия, дата]}
    return [row for row in yt.read_table(RESULT_TABLE, format=yt.JsonFormat(attributes={"json": False}))]


def write_new_results(results):
    yt.write_table(RESULT_TABLE, results, format="json")


def update_results(previous_results, new_results):
    """
    Обновляем уже существующие результаты новыми, и формируем в формате готовым к отправке в YT:
    [{'check_name': '', 'version': '', 'check_date': ''}] - имена столбцов
    """
    updated_results = [nr[0] for nr in new_results]
    return [result for result in previous_results if result['check_name'] not in updated_results] +\
           [{'check_name': nr[0],
             'version': nr[1]['version'],
             'check_date': nr[1]['check_date'],
             'check_built': False} for nr in new_results]


def send_mail(text):
    from email.mime.multipart import MIMEMultipart
    from email.mime.text import MIMEText
    from email.utils import COMMASPACE, formatdate

    sender = 'no-reply@yandex-team.ru'
    to = 'antiadb@yandex-team.ru'

    msg = MIMEMultipart()
    msg["Subject"] = 'Обновления в Адблоках'
    msg["To"] = COMMASPACE.join(to)
    msg["From"] = sender
    msg['Date'] = formatdate(localtime=True)

    msg.attach(MIMEText(text, _charset='utf-8'))
    smtp = SMTP(host='yabacks.yandex.ru', port=25)
    smtp.sendmail(sender, to, msg.as_string())
    smtp.quit()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Anti-AdBlock analytic tool. Gathering information about adblocks/browsers/etc...')
    parser.add_argument('--yt_token', metavar='TOKEN', help='Using yt for storing checks results.')
    parser.add_argument('--local_run', action='store_true', help='Run without sending results to YT')
    args = parser.parse_args()

    logging.basicConfig(format=u'%(levelname)-8s [%(asctime)s]: %(message)s', level=logging.INFO)
    local_run = args.local_run or getenv('LOCAL_RUN', 'False') == 'True'

    # создаем yt клиент
    token = args.yt_token or getenv('YT_TOKEN', None)
    # если нет токена то все результаты считаются новые, так как мы не можем загрузить предыдущие
    previous_results = {}
    if token is not None:
        config_yt_client(token)
        # грузим результаты предыдущих прогонов
        previous_results = get_previous_result()

    results = []
    # заимпортим все модули
    for module in MODULES:
        # загрузка данных = выполнение get_check_result() в каждом модуле
        check_module = getattr(getattr(modules, module), module)(logging)
        check_result = check_module.get_check_result()

        # фильтрация результатов
        results += check_module.filter_results(check_result, previous_results)

    if results:
        # краткая запись в лог
        for k, v in results:
            logging.info('Find new information of {}, url: {}'.format(k, v['url']))
        # сформировать отчет и отправить в письме
        msg = '\n\n\n'.join(['NEW VERSION OF {}, {}\nURL: {}\n\nDESCRIPTION:\n{}'.format(k, v['version'], v['url'], v['description'].encode('utf-8'))
                             for k, v in results])

        send_mail(msg)

        # сохраняем в yt новые результаты
        if token and not local_run:
            # мерджим результаты
            results_to_yt = update_results(previous_results, results)
            # записываем обратно
            write_new_results(results_to_yt)
