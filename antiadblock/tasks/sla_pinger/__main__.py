# coding=utf-8
import os
from datetime import datetime, timedelta

import startrek_client
from statface_client import StatfaceClient, STATFACE_PRODUCTION, constants as stat_consts

from antiadblock.libs.utils.utils import get_abc_duty

SUPPORT_MONEY_LOSS_REPORT = 'AntiAdblock/support_money_loss'
DATETIME_FORMAT = '%Y-%m-%dT%H:%M:%S'

STARTREK_TOKEN = os.getenv('STARTREK_TOKEN')
STAT_TOKEN = os.getenv('STAT_TOKEN')
ABC_TOKEN = os.getenv('ABC_TOKEN')

MARKER_TEXT = 'PING_SLA'
COMMENT_TEXT_BASE = MARKER_TEXT + '\n\nТикет превысил SLA, предприми действия, чтобы вернуть тикет в рамки SLA в ' \
                                  'соответсвии с регламентом дежурств!'
MONEY_DISCLAIMER = '\n**В день недозарабатываем около {0:,} !!!**'

SLA_VIOLATED_TAG = 'sla_violated'


def create_comment(issue, duty_login, money_by_tickets):
    text = COMMENT_TEXT_BASE
    if issue.key in money_by_tickets:
        text += MONEY_DISCLAIMER.format(int(money_by_tickets[issue.key]['loss'])).replace(',', ' ')
    issue.comments.create(text=text, summonees=[duty_login])


def get_money_by_tickets():
    stat_client = StatfaceClient(host=STATFACE_PRODUCTION, oauth_token=STAT_TOKEN)
    money_loss_report = stat_client.get_report(SUPPORT_MONEY_LOSS_REPORT)
    money_loss_data = money_loss_report.download_data(
        scale=stat_consts.DAILY_SCALE,
        date_max=datetime.now().strftime(DATETIME_FORMAT),
        date_min=(datetime.now() - timedelta(days=2)).strftime(DATETIME_FORMAT)
    )
    money_by_tickets = {}
    for item in money_loss_data:
        ticket_id = item['ticket']
        date_ms = money_by_tickets.get(ticket_id, {'fielddate__ms': 0})['fielddate__ms']
        if item['fielddate__ms'] > date_ms:
            money_by_tickets[ticket_id] = item
    return money_by_tickets


def is_sla_broken(issue):
    if not issue.sla:
        return False
    for sla in issue.sla:
        if sla['violationStatus'] == 'FAIL_CONDITIONS_VIOLATED' and sla['clockStatus'] == 'STARTED':
            return True
    return False


def add_violated_tag(issue):
    if SLA_VIOLATED_TAG not in issue.tags:
        issue.update(tags=issue.tags + [SLA_VIOLATED_TAG])


def remove_violated_tag(issue):
    new_tags = [tag for tag in issue.tags if tag != SLA_VIOLATED_TAG]
    issue.update(tags=new_tags)


if __name__ == "__main__":
    st_client = startrek_client.Startrek(useragent='python', token=STARTREK_TOKEN)
    money_by_tickets = get_money_by_tickets()
    all_issues = st_client.issues.find(query='Queue: ANTIADBSUP and status: !closed')
    dt_limit = datetime.now() - timedelta(days=1)
    duty_login, _ = get_abc_duty(ABC_TOKEN)
    for issue in all_issues:
        if is_sla_broken(issue):
            add_violated_tag(issue)
            has_outdated_comment = True
            for comment in list(issue.comments.get_all())[::-1]:
                created = datetime.strptime(comment.createdAt.split('.')[0], DATETIME_FORMAT)
                if MARKER_TEXT in comment.text:
                    if created > dt_limit:
                        has_outdated_comment = False
                    break
            if has_outdated_comment:
                create_comment(issue, duty_login, money_by_tickets)
        else:
            remove_violated_tag(issue)
