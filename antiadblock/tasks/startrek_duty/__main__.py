# -*- coding: utf-8 -*-
import os
import datetime
import collections

import startrek_client
import statface_client

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.tools.common_configs import STARTREK_DATETIME_FMT


def send_to_stat(data, report_name):
    client = statface_client.StatfaceClient(
        host=statface_client.STATFACE_PRODUCTION,
        oauth_token=os.getenv('STAT_TOKEN')
    )
    report = client.get_report('AntiAdblock/{}'.format(report_name))
    report.upload_data(scale='daily', data=data)


def get_date(t):
    return get_datetime(t).date()


def get_datetime(t):
    return datetime.datetime.strptime(t.split('.')[0], '%Y-%m-%dT%H:%M:%S')


def get_issues_without_activity(st_client):
    issues = st_client.issues.find(query='queue: ANTIADBSUP AND (status: open OR status: InProgress)')
    issues_list = list()

    today = datetime.date.today()

    for i in issues:
        last_change = i.lastCommentUpdatedAt or i.createdAt
        if today - get_date(last_change) > datetime.timedelta(weeks=2):
            issues_list.append(i)
    return issues_list


def get_average_ticket_lifetime(st_client):
    issues = st_client.issues.find(query='queue: ANTIADBSUP and Created: > 25-12-2017 and status: closed')
    return sum(map(lambda i: int((get_date(i.resolvedAt) - get_date(i.createdAt)).total_seconds()/3600), issues))/len(issues)


def get_average_ticket_reaction_time(st_client):
    issues = st_client.issues.find(query='queue: ANTIADBSUP and Created: > 25-12-2017')
    reaction_time_sum = 0
    for issue in issues:
        if issue.lastCommentUpdatedAt is None:
            if getattr(issue, 'resolvedAt', None) is None:
                # Issue without reaction: reaction_time = now() - createdAt
                reaction_time_sum += (datetime.datetime.now() - get_datetime(issue.createdAt)).total_seconds()
            else:
                # Issue without comments but with resolution: reaction_time = lifetime
                reaction_time_sum += (get_datetime(issue.resolvedAt) - get_datetime(issue.createdAt)).total_seconds()
        else:
            # Issue with comments: reaction_time = first_comment.createdAt - createdAt
            try:
                reaction_time_sum += (get_date(issue.comments.get_all()._data[0].createdAt) - get_date(issue.createdAt)).total_seconds()
            except AttributeError:
                # Issue with deleted comment has lastCommentUpdatedAt but doesn't has comments
                reaction_time_sum += (get_datetime(issue.resolvedAt) - get_datetime(issue.createdAt)).total_seconds()
    return reaction_time_sum/(len(issues) * 3600.0)


def get_sla_violation_statisitcs(st_client, yesterday):

    yesterday_date = datetime.date(*map(int, yesterday.split('-')))
    monday = yesterday_date - datetime.timedelta(days=yesterday_date.weekday())
    issues = st_client.issues.find(
        query='Queue: ANTIADBSUP and Priority: Normal, Critical, Blocker and (status: !closed or resolved: >= {})'.format(
            monday.strftime('%Y-%-m-%d')))
    data = collections.defaultdict(lambda: {'violated': 0,
                                            'not_violated': 0,
                                            'violation_time': list()})
    for issue in issues:
        issue_sla = issue.sla or []
        for sla in issue_sla:
            if sla['violationStatus'] == 'NOT_VIOLATED':
                data[sla['settingsId']]['not_violated'] += 1
            else:
                data[sla['settingsId']]['violated'] += 1
                time_spent = sla['spent']
                sla_start_time = datetime.datetime.strptime(sla['startedAt'].split('.')[0],
                                                            STARTREK_DATETIME_FMT)
                if time_spent is not None:
                    sla_stop_time = sla_start_time + datetime.timedelta(milliseconds=time_spent)
                    sla_failed_at = datetime.datetime.strptime(sla['failAt'].split('.')[0], STARTREK_DATETIME_FMT)
                    violation_time = (sla_stop_time - sla_failed_at).total_seconds()
                else:
                    # Если над задачей не начинали работу, то поле spent будет пустым
                    # В этом случае считаем за нарушение время от старта таймера до конца вчерашнего дня
                    violation_time = (datetime.datetime.strptime(yesterday + 'T23:59:59',
                                                                 STARTREK_DATETIME_FMT) - sla_start_time).total_seconds()
                data[sla['settingsId']]['violation_time'].append(violation_time)

        for d in data.values():
            try:
                violation_time = d['violation_time']
                d['avg_violation_time'] = sum(violation_time) / (d['violated'] + d['not_violated'])
                # Мы получили время в секундах, а в отчет будем добавлять минуты
                d['avg_violation_time'] /= 60
            except ZeroDivisionError:
                d['avg_violation_time'] = 0
    return data


def calculate_ST_statistics():
    statface_data = dict(
        ST_duty=list(),
        tickets_time=list(),
        sla_violation=list()
    )
    st_client = startrek_client.Startrek(useragent='python', token=os.getenv('STARTREK_TOKEN'))
    yesterday = (datetime.datetime.now() - datetime.timedelta(days=1)).strftime("%Y-%m-%d")
    # Считаем статистику по нарушениям SLA
    sla_violation = get_sla_violation_statisitcs(st_client, yesterday)
    for sla_type, data in sla_violation.iteritems():
        statface_data['sla_violation'].append({'fielddate': yesterday,
                                               'group': '\tyesterday\tsla\t{}\tviolated\t'.format(sla_type),
                                               'count': data.get('violated', 0)})
        statface_data['sla_violation'].append({'fielddate': yesterday,
                                               'group': '\tyesterday\tsla\t{}\tnot_violated\t'.format(sla_type),
                                               'count': data.get('not_violated', 0)})
        statface_data['sla_violation'].append({'fielddate': yesterday,
                                               'group': '\tyesterday\tsla\t{}\tavg_violation_time_minutes\t'.format(sla_type),
                                               'count': data.get('avg_violation_time', 0)})
    # считаем среднее время жизни тикета
    avg_lifetime = get_average_ticket_lifetime(st_client)
    statface_data['tickets_time'].append({'fielddate': yesterday, 'group': '\tyesterday\tavg_lifetime\t', 'hours': avg_lifetime})
    # считаем среднее время реакции на тикет
    avg_reaction_time = get_average_ticket_reaction_time(st_client)
    statface_data['tickets_time'].append({'fielddate': yesterday, 'group': '\tyesterday\tavg_reaction_time\t', 'hours': avg_reaction_time})
    # считаем количество закрытых тикетов только за вчерашний день
    issues = st_client.issues.find(query='queue: ANTIADBSUP AND Resolved:{} And status: closed'.format(yesterday))
    statface_data['ST_duty'].append({'fielddate': yesterday, 'group': '\tyesterday\tclosed\t', 'count': len(issues)})
    # считаем количество открытых тикетов только за вчерашний день
    issues = st_client.issues.find(query='queue: ANTIADBSUP AND created:{}'.format(yesterday))
    statface_data['ST_duty'].append({'fielddate': yesterday, 'group': '\tyesterday\topened\t', 'count': len(issues)})
    # считаем количество тикетов, в которых не было активности последние 2 недели
    issues = get_issues_without_activity(st_client)
    statface_data['ST_duty'].append({'fielddate': yesterday, 'group': '\tyesterday\twithoutActivity\t', 'count': len(issues)})
    # считаем количество незакрытых тикетов с разными тегами и статусами
    issues = st_client.issues.find(query='queue: ANTIADBSUP AND Status: !Closed AND Status: !wontfix')
    statface_data['ST_duty'].append({'fielddate': yesterday, 'group': '\tyesterday\ttotal\t', 'count': len(issues)})
    statuses = collections.Counter([issue.status.key for issue in issues])
    for status, count in statuses.items():
        statface_data['ST_duty'].append({'fielddate': yesterday, 'group': '\tyesterday\t{}\t'.format(status), 'count': count})
    tags = collections.Counter([tag for issue in issues for tag in issue.tags])
    for tag, count in tags.items():
        statface_data['ST_duty'].append({'fielddate': yesterday, 'group': '\ttags\t{}\t'.format(tag), 'count': count})
    return statface_data


if __name__ == '__main__':
    logger = create_logger('startrek_duty')
    ticket_statistics = calculate_ST_statistics()
    for report_name, data in ticket_statistics.iteritems():
        logger.info('Send data to {}'.format(report_name))
        send_to_stat(data, report_name)
