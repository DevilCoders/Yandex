# coding=utf8
import os
import json

import requests
from datetime import datetime
from startrek_client import Startrek

from antiadblock.tasks.tools.logger import create_logger
from antiadblock.tasks.money_tickets_updater.plugins import plugins

STARTREK_TOKEN = os.getenv('STARTREK_TOKEN')
logger = create_logger('money_tickets_updater')
TAG = "ready4autofill"


def get_open_tickets(st_client):
    issues = st_client.issues.find(
        query='Queue: ANTIADBSUP and status: !closed and Tags: ' + TAG
    )
    return issues


def update_checklist(issue_key, add_items):
    items = [dict(text=item, checked=False) for item in add_items]
    r = requests.post(
        'https://st-api.yandex-team.ru/v2/issues/{}/checklistItems'.format(issue_key),
        data=json.dumps(items),
        headers={
            'Authorization': 'OAuth {}'.format(STARTREK_TOKEN),
            'Content-type': 'application/json'
        }
    )
    try:
        r.raise_for_status()
        return True
    except Exception:
        logger.exception("Failed update_checklist.\n")
        return False


def update_description(st_client, issue):
    service_id = issue.components[0].name
    start_time = datetime.strptime((issue.incidentStart or issue.createdAt).split('.')[0], '%Y-%m-%dT%H:%M:%S')
    description = issue.description or str()
    success_update = True
    checklist_items = [item["text"].encode('utf-8') for item in issue.checklistItems]
    add_checklist_items = []

    for plugin_name, plugin in plugins.items():
        if plugin_name in checklist_items:
            continue
        try:
            plugin_description = plugin(service_id, start_time)
            if plugin_description is None:
                continue
            description += "<{" + plugin_name.decode('utf-8') + "\n" + plugin_description.decode('utf-8') + "}>\n\n"
            if plugin_name not in checklist_items:
                add_checklist_items.append(plugin_name)
        except Exception:
            logger.exception("Plugin {} run for service_id {} failed for\n".format(plugin.__name__, service_id))
            success_update = False

    success_update = update_checklist(issue.key, add_checklist_items) and success_update

    issue = st_client.issues[issue.key]
    if success_update:
        issue.tags.remove(TAG)
    if issue.description != description:
        issue.update(description=description, tags=issue.tags)


if __name__ == "__main__":
    st_client = Startrek(useragent='python', token=STARTREK_TOKEN)
    for issue in get_open_tickets(st_client):
        issue_key = issue.key
        try:
            update_description(st_client, issue)
        except Exception as e:
            logger.exception("Failed update {}\n".format(issue_key))
