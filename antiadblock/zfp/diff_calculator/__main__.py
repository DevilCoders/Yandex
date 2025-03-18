# coding=utf-8

import os
import smtplib
import argparse

from startrek_client import Startrek
from antiadblock.tasks.tools.logger import create_logger
from adv.pcode.zfp.diff_calculator.lib.helper import calculate_diff, create_message, create_message_content, upload_file_to_sandbox, MSG_WITH_RESOURCE_URL_TMPL

logger = create_logger(__file__)

EMAIL_FROM = 'no-reply@yandex-team.ru'
EMAIL_TO = 'antiadb@yandex-team.ru'
EMAIL_COPY = 'dridgerve@yandex-team.ru'

RESOURCE_TYPE = 'AAB_ZFP_DIFF_RESULT'
SB_OWNER = 'ANTIADBLOCK'
STARTREK_QUEUE = 'ANTIADBALERTS'


if __name__ == '__main__':
    crit_description = ''
    parser = argparse.ArgumentParser()
    parser.add_argument('--source', required=True, help='Directory with calculation results')
    parser.add_argument('--self_url', required=True, help='Link to workflow')
    parser.add_argument('--ts_from', required=True, help='Timestamp from', type=int)
    parser.add_argument('--ts_to', required=True, help='Timestamp to', type=int)
    parser.add_argument('--juggler_host', required=True, help='Juggler host')

    STARTREK_TOKEN = os.getenv('STARTREK_TOKEN')

    args = parser.parse_args()
    st_client = Startrek(useragent='python', token=STARTREK_TOKEN)

    diff_data, has_problem = calculate_diff(args.source, STARTREK_QUEUE, st_client, args.ts_from, args.ts_to)
    message_content = create_message_content(diff_data, args.self_url, STARTREK_QUEUE)
    # save resource
    if diff_data:
        filename = os.path.abspath(os.path.join(args.source, 'diff.html'))
        with open(filename, 'w') as fout:
            fout.write(message_content)
        resource_id = upload_file_to_sandbox(filename, args.juggler_host, resource_type=RESOURCE_TYPE, owner=SB_OWNER)
        message_content = MSG_WITH_RESOURCE_URL_TMPL.format(resource_id=resource_id)

    msg = create_message(message_content, args.juggler_host, email_to=EMAIL_TO, email_from=EMAIL_FROM, email_copy=EMAIL_COPY)
    smtp = smtplib.SMTP(host='outbound-relay.yandex.net', port=25)
    smtp.send_message(msg)
    smtp.quit()

    # Фейлим граф, если есть проблемы
    if has_problem:
        raise Exception('There some problems with this calculation. We mailed you additional info')
