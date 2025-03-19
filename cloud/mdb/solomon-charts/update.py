#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import os
import json
import click
import copy
from jinja2 import Environment, BaseLoader, StrictUndefined
import requests
import urllib3
import re

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)

# logging.basicConfig()
# logging.getLogger().setLevel(logging.DEBUG)
# requests_log = logging.getLogger("requests.packages.urllib3")
# requests_log.setLevel(logging.DEBUG)
# requests_log.propagate = True

ENDPOINT = 'https://solomon.yandex.net/api/v2'
DEBUG = 0

entity_paths = {
    'dashboard': 'dashboards/{}.json',
    'graph': 'graphs/{}.json',
    'alert': 'alerts/{}.json',
    'notificationChannel': 'notification_channels/{}.json',
}

env = Environment(loader=BaseLoader, undefined=StrictUndefined,
                  variable_start_string='<<',
                  variable_end_string='>>')


def load_alert_program(entity, entity_name):
    keyword = "{{alert_program}}"
    if keyword not in entity:
        return entity
    path = 'alerts/{}-program.json'.format(entity_name)
    with open(path, 'r') as fh:
        raw = fh.read()
        raw = raw.replace("\n", "\\n").replace('\r', '')
        return entity.replace(keyword, raw)


def load_entity(path, entity_type, entity_name, projectId, prefix, dry_run, env_keys):
    print("Loading file: {}".format(path))
    with open(path, 'r') as fh:
        raw = fh.read()
        raw = raw.replace('{{projectId}}', projectId)
        raw = raw.replace('{{prefix}}', prefix)
        if entity_type is 'alert':
            raw = load_alert_program(raw, entity_name)
        j_template = env.from_string(raw)
        print(env_keys)
        raw = j_template.render(env_keys)
        if dry_run:
            print(raw)
        return json.loads(raw)


def is_equal(old, new):
    old = copy.deepcopy(old)
    new = copy.deepcopy(new)
    # remove dynamic fields
    for key in ['createdBy', 'createdAt', 'version', 'updatedAt', 'updatedBy']:
        for d in [old, new]:
            if d.get(key) is not None:
                del d[key]

    # Some alarms specific treatment
    # No need to check 'state' field of alarms
    for key in ['state']:
        if key not in new and key in old:
            old.pop(key, None)

    # Special treatment for graphs
    if 'elements' in old and 'elements' in new and \
        old['elements'] != new['elements'] and \
        isinstance(old['elements'], list) and \
        isinstance(new['elements'], list) and \
        len(old['elements']) == len(new['elements']):
        for i in range(len(new['elements'])):
            for d in [old['elements'][i], new['elements'][i]]:
                if d['type'] == 'SELECTORS':
                    d.pop('expression', None)
                elif d['type'] == 'EXPRESSION':
                    d.pop('selectors', None)
                if d.get('link', None) == '':
                    d.pop('link', None)

    return old == new


def update_entity(endpoint, headers, entity_name, projectId, entity_type,
                  prefix, dry_run, env_keys):
    session = requests.Session()
    path = entity_paths[entity_type].format(entity_name)
    entity = load_entity(path, entity_type, entity_name, projectId, prefix, dry_run, env_keys)
    entity_id = '{prefix}{id}'.format(prefix=prefix, id=entity['id'])
    entity['id'] = entity_id

    r = session.get(
        '{endpoint}/projects/{projectId}/{entity}s/{entityId}'.format(
            endpoint=endpoint,
            projectId=projectId,
            entity=entity_type,
            entityId=entity_id),
        verify=False,
        headers=headers)
    if r.status_code == 404:
        if dry_run is True:
            print("POST: {}".format(path))
            print("ENTITY:\n{}".format(json.dumps(entity, indent=4,
                                                  sort_keys=True)))
            print("-------")
            return
        r = session.post(
            '{endpoint}/projects/{projectId}/{entity}s'.format(
                endpoint=endpoint,
                projectId=projectId,
                entity=entity_type),
            headers=headers,
            verify=False,
            data=json.dumps(entity))
        if r.status_code == 200:
            print('{path} created'.format(path=path))
            return
        else:
            print('{path} create error, http_code: {code}, '
                  'response: {response}'.format(
                path=path, code=r.status_code, response=r.json()))
            r.raise_for_status()
    elif r.status_code == 200:
        old = r.json()
        entity['version'] = int(old['version'])

        if is_equal(old, entity):
            print('{path} skipped, no difference'.format(path=path))
            return
        if dry_run:
            print("PUT: {}".format(path))
            print("ENTITY:\n{}".format(json.dumps(entity, indent=4,
                                                  sort_keys=True)))
            if DEBUG == 1:
                print("_______")
                print("Entity diff:")
                for key in old:
                    if key not in entity:
                        print('Deleted keys: "{}": {},'.format(key, json.dumps(old[key], indent=4, sort_keys=True)))
                for key in entity:
                    if key not in old:
                        print('New keys: "{}": {},'.format(
                            key, json.dumps(entity[key], indent=4, sort_keys=True)))
                for key in entity:
                    if entity[key] != old[key]:
                        print("Diff keys: {}".format(key))
            print("-------")
            return
        r = session.put(
            '{endpoint}/projects/{projectId}/{entity}s/{entityId}'.format(
                endpoint=endpoint,
                projectId=projectId,
                entity=entity_type,
                entityId=entity_id),
            headers=headers,
            verify=False,
            data=json.dumps(entity))
        if r.status_code == 200:
            print('{path} updated'.format(path=path))
        else:
            print('{path} update error, http_code: {code}, '
                  'response: {response}'.format(
                path=path, code=r.status_code, response=r.json()))
            r.raise_for_status()
    else:
        print('{path} get error, http_code: {code}, '
              'response: {response}'.format(path=path, code=r.status_code, response=r.json()))
        r.raise_for_status()


KIND2SOLOMON_KIND = {
    'dashboards': 'dashboard',
    'graphs': 'graph',
    'notification_channels': 'notificationChannel',
    'alerts': 'alert',
}


@click.command()
@click.option('--project-name', required=True, help='Project name (internal-mdb or yandexcloud)')
@click.option('--env', required=True, help='Name of an environment')
@click.option('--token', required=False, help='OAuth token')
@click.option('--iam', required=False, help='IAM token')
@click.option('--dry-run', required=False, is_flag=True, help='Run in dry run mode')
@click.option('--kind', required=False, type=click.Choice(list(KIND2SOLOMON_KIND)), help='Update only kind entries')
@click.option('--pattern', required=False, help='Regexp pattern to filter entries to update')
def main(project_name, env, token, iam, dry_run, kind, pattern):
    if iam:
        auth = 'Bearer {token}'.format(token=iam)
    else:
        if token is None:
            token = os.environ.get('TOKEN')
        auth = 'OAuth {token}'.format(token=token)

    headers = {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
        'Authorization': auth,
    }

    update_entries_kinds = list(KIND2SOLOMON_KIND.keys())
    if kind is not None:
        update_entries_kinds = [kind]

    with open('config.json') as fh:
        config = json.load(fh)

    project = config[project_name]
    endpoint = os.environ.get(
        'ENDPOINT',
        project.get('endpoint', ENDPOINT))
    prefix = project.get('prefix', '')
    solomon_project = project.get('solomon_project', project_name)
    env_keys = project['envs'][env]
    for entry_kind in update_entries_kinds:
        for entry in project.get(entry_kind, []):
            if not pattern or re.search(pattern, entry):
                update_entity(
                    endpoint,
                    headers,
                    entry,
                    solomon_project,
                    KIND2SOLOMON_KIND[entry_kind],
                    prefix,
                    dry_run,
                    env_keys)


if __name__ == '__main__':
    main()
