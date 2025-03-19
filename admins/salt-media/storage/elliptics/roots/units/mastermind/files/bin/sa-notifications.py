#!/usr/bin/python
# -*- coding: utf-8 -*-

from __future__ import absolute_import
from __future__ import unicode_literals
import argparse
import datetime
import logging
import json
import requests
import time
from yql.api.v1.client import YqlClient
from collections import defaultdict

from requests.packages.urllib3.exceptions import InsecureRequestWarning

requests.packages.urllib3.disable_warnings(InsecureRequestWarning)


logger = logging.getLogger('notifications')

YT_CLUSTER = 'hahn'
YT_TOKEN = "{{ pillar['yav']['sa_notifications_yt_token'] }}"
ST_TOKEN = "{{ pillar['robot_storage_duty_yav']['startrek-oauth'] }}"
ST_QUEUES = ["MDS", "MDSTODO", "MDSSUPPORT", "CLOUD"]
ST_PING_QUEUES = ["MDS"]
LOGINS = [
    "kanst9",
    "hotosho",
    "kdmitriy",
    "izhidkov",
    "arstotzkan",
    "denkoren",
    "dimovvasiliy",
    "vkokarovtsev",
    "MdsMonitoring",
]
ESCALATIONS_COUNT_THRESHOLD = 3


def get_args():
    parser = argparse.ArgumentParser()
    parser.add_argument('-l', '--log-file', default='/var/log/sa/notifications.log')
    parser.add_argument(
        '-d', '--days', type=int, default=1, help='Get notifications for N previous days',
    )
    parser.add_argument(
        '-p', '--ping', action='store_true', help='Ping st tickets for top notifications',
    )
    parser.add_argument(
        '-c',
        '--custom-logins',
        help='Logins for notification (comma-separated values can be used). Default: {}'.format(
            LOGINS
        ),
    )
    args = parser.parse_args()
    return args


def setup_logging(log_file):
    _format = logging.Formatter("[%(asctime)s] [%(name)s] %(levelname)s: %(message)s")
    _handler = logging.FileHandler(log_file)
    _handler.setFormatter(_format)
    logging.getLogger().setLevel(logging.DEBUG)
    logging.getLogger().addHandler(_handler)


class Startrek(object):
    search_url = 'https://st-api.yandex-team.ru/v2/issues'
    ping_url = 'https://st-api.yandex-team.ru/v2/issues/{}/comments'
    issue_url = 'https://st-api.yandex-team.ru/v2/issues/{}'

    """
    {
        "MDS-12345": {
            "elliptics-cloud:mm-namespace-space": {
                "phone_escalation": 33,
                "telegram": 4
            }
            "elliptics-proxy:get-5xx": {
                "telegram": 5
            }
        },
        "MDS-12346": {
            "elliptics-proxy:get-5xx": {
                "phone_escalation": 3,
                "sms": 8
            }
        },

    }
    """
    ping_issues = defaultdict(lambda: defaultdict(lambda: defaultdict(int)))

    def __init__(self):
        self.headers = dict(Authorization='OAuth {}'.format(ST_TOKEN))
        self.queues = ST_QUEUES
        self.st_ping_queues = ST_PING_QUEUES

    def get_issue_by_tag(self, tag, count, escalation_method):
        # "https://st-api.yandex-team.ru/v2/issues?filter=queue:MDS&filter=tags:mastermind&filter=resolution:empty()&perPage=1"
        for queue in self.queues:
            params = dict(
                filter=['queue:{}'.format(queue), 'tags:{}'.format(tag), 'resolution:empty()'],
                perPage='3',
            )
            resp = requests.get(
                url=self.search_url, params=params, headers=self.headers, verify=False
            )
            # st have 5rps limit for searches per account
            time.sleep(0.3)
            resp.raise_for_status()
            data = resp.json()
            if data:
                for issue in data:
                    self.ping_issues[issue['key']][tag][escalation_method] = count
                assignee = data[0].get('assignee', {}).get('display', 'Нет исполнителя')
                return "{} {}".format(data[0]['key'], assignee)

        return "нет тикета"

    def ping_all_issues(self, date_string):
        for issue, alerts in self.ping_issues.iteritems():
            resp = requests.get(
                url=self.issue_url.format(issue), headers=self.headers, verify=False
            )
            issue_data = resp.json()
            if issue_data['queue']['key'] not in self.st_ping_queues:
                return

            count_all = 0
            alerts_list = []
            for alert, escalations in alerts.iteritems():
                for escalation, count in escalations.iteritems():
                    alerts_list.append(" - {}: {} {} раз(а)".format(alert, escalation, count))
                    count_all += count
            p_url = self.ping_url.format(issue)
            assignee_id = issue_data.get('assignee', {}.get('id', None))
            if assignee_id:
                comment = dict(
                    text="За {} мониторинги по данной проблеме беспокоили людей {} раз(а):\n{}\nПришло время заняться задачей.".format(
                        date_string, count_all, "\n".join(alerts_list)
                    ),
                    summonees=[issue_data['assignee']['id']],
                )
            else:
                comment = dict(
                    text="За {} мониторинги по данной проблеме беспокоили людей {} раз(а):\n{}\nПришло время заняться задачей.".format(
                        date_string, count_all, "\n".join(alerts_list)
                    ),
                )
            requests.post(url=p_url, json=comment, headers=self.headers, verify=False)


def get_notifications(logins, days):
    tmp = """
PRAGMA AnsiInForEmptyOrNullableItemsCollections;
select
    DISTINCT checks, method, desc_hash
from
    range(`home/logfeller/logs/juggler-banshee-log/1d`, `{start}`, `{finish}`)
where
    login in ({logins})
    and method in ('phone_escalation','sms','telegram') and event_type = 'message_processed'
    """

    finish = datetime.date.today() - datetime.timedelta(1)
    start = finish - datetime.timedelta(days - 1)
    logins = ','.join(["'{}'".format(l) for l in logins])

    query = tmp.format(
        start=start.strftime("%Y-%m-%d"), finish=finish.strftime("%Y-%m-%d"), logins=logins
    )

    client = YqlClient(db=YT_CLUSTER, token=YT_TOKEN)
    request = client.query(query, syntax_version=1)
    request.run()
    return request.full_dataframe.values.tolist()


def to_json(notifications):
    result = defaultdict(lambda: defaultdict(int))

    for data in notifications:
        check = data[0]
        method = data[1]
        result[method][check] += 1

    return result


def print_result(
    result, days, ping_st=False, escalation_methods=['telegram', 'sms', 'phone_escalation']
):
    if days == 1:
        finish = datetime.date.today() - datetime.timedelta(1)
        date_string = finish.strftime("%Y-%m-%d")
    else:
        finish = datetime.date.today() - datetime.timedelta(1)
        start = finish - datetime.timedelta(days - 1)
        date_string = "{} - {}".format(start.strftime("%Y-%m-%d"), finish.strftime("%Y-%m-%d"))
    print "<b>Вестник мониторинга за {}:</b>".format(date_string)
    st = Startrek()
    for escalation_method in escalation_methods:
        print_dict = result.get(escalation_method, {})
        print_list = sorted(print_dict.items(), key=lambda x: x[1], reverse=True)
        total = 0
        for check_count in print_list:
            total += check_count[1]

        print_list = [item for item in print_list if item[1] >= ESCALATIONS_COUNT_THRESHOLD]
        print "<b>{}:</b> {} total".format(escalation_method, total)
        print "<pre language='c++'>"
        if print_list:
            for check_count in print_list:
                st_issue = st.get_issue_by_tag(check_count[0], check_count[1], escalation_method)
                print "{:^4}{} {}".format(check_count[1], check_count[0], st_issue)
        else:
            print " Збс, {} в пределах нормы!".format(escalation_method)
        print "</pre>"
        if escalation_method != escalation_methods[-1]:
            print "SPLIT"
    if ping_st:
        st.ping_all_issues(date_string)


def main():
    args = get_args()
    setup_logging(args.log_file)
    logger.debug('Start with arguments: {}'.format(args))

    if args.custom_logins:
        custom_logins = args.custom_logins.split(',')
        notifications = get_notifications(custom_logins, args.days)
    else:
        notifications = get_notifications(LOGINS, args.days)

    result = to_json(notifications)
    result = json.loads(json.dumps(result))

    if args.custom_logins:
        print_result(result, days=args.days, ping_st=args.ping, escalation_methods=['telegram'])
    else:
        print_result(result, days=args.days, ping_st=args.ping)


if __name__ == '__main__':
    main()
