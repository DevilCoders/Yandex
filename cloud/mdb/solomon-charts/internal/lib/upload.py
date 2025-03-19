#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import copy
import logging
import json
import os
from pathlib import Path
import requests
import urllib3
import re

from . import KIND2SOLOMON_KIND

urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)


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


def load_entities(env, kind, pattern):
    logging.info("Loading entities: env - %s, kind - %s" % (env, kind))
    dir_path = os.path.join(os.getcwd(), "out", env, kind)
    if os.path.exists(dir_path):
        assert os.path.isdir(dir_path)
    else:
        return []

    files = [os.path.join(dir_path, x) for x in list(Path(dir_path).rglob("*.json"))]

    if pattern:
        files = [file for file in files if re.search(pattern, file)]

    return [Path(x).read_text() for x in files]


def json_validation_err(text):
    try:
        json.loads(text)
        return None
    except ValueError as e:
        return 'invalid json: %s \n %s' % (e, text[0:min(len(text), 100)])


def update_entities(env_name, env, entity_kind, pattern, headers, dry_run):
    session = requests.Session()
    entities = load_entities(env_name, entity_kind, pattern)
    entity_type = KIND2SOLOMON_KIND[entity_kind]
    endpoint = env["endpoint"]
    project_id = env["project_id"]
    for entity in entities:
        validation_msg = json_validation_err(entity)
        if validation_msg is not None:
            logging.error("Invalid entity:\n%s" % validation_msg)
            logging.error("Stop")
            return
        update_entity(entity, endpoint, session, entity_type, headers, project_id, dry_run)


def update_entity(entity, endpoint, session, entity_type, headers, project_id, dry_run):
    ej = json.loads(entity)
    entity_id = ej["id"]
    logging.info("Updating entity: " + "{}s/{}".format(entity_type, entity_id))
    r = session.get(
        '{endpoint}/projects/{projectId}/{entity}s/{entityId}'.format(
            endpoint=endpoint,
            projectId=project_id,
            entity=entity_type,
            entityId=entity_id),
        verify=False,
        headers=headers)
    if r.status_code == 404:
        if dry_run is True:
            logging.info("dry_run")
            logging.info("POST: {}".format(entity_id))
            logging.info("ENTITY:\n{}".format(json.dumps(entity, indent=4, sort_keys=True)))
            logging.info("-------")
            return
        r = session.post(
            '{endpoint}/projects/{projectId}/{entity}s'.format(
                endpoint=endpoint,
                projectId=project_id,
                entity=entity_type),
            headers=headers,
            verify=False,
            data=entity)
        if r.status_code == 200:
            logging.info('{entity_id} created'.format(entity_id=entity_id))
            return
        else:
            logging.info('{entity_id} create error, http_code: {code}, response: {response}'.format(entity_id=entity_id,
                                                                                                    code=r.status_code,
                                                                                                    response=r.json()))
            r.raise_for_status()
    if r.status_code == 200:
        old = r.json()
        ej['version'] = int(old['version'])

        if is_equal(old, ej):
            logging.info('{entity_id} skipped, no difference'.format(entity_id=entity_id))
            return
        if dry_run:
            logging.info("dry_run")
            logging.info("PUT: {}".format(entity_id))
            logging.info("ENTITY:\n{}".format(ej))
            logging.info("_______")
            logging.info("Entity diff:")
            for key in old:
                if key not in ej:
                    logging.info('Deleted keys: "{}": {},'.format(key, json.dumps(old[key], indent=4, sort_keys=True)))
            for key in ej:
                if key not in old:
                    logging.info('New keys: "{}": {},'.format(
                        key, json.dumps(ej[key], indent=4, sort_keys=True)))
            for key in ej:
                if ej[key] != old[key]:
                    logging.info("Diff keys: {}".format(key))
            logging.info("-------")
            return
        r = session.put(
            '{endpoint}/projects/{projectId}/{entity}s/{entityId}'.format(
                endpoint=endpoint,
                projectId=project_id,
                entity=entity_type,
                entityId=entity_id),
            headers=headers,
            verify=False,
            data=json.dumps(ej))
        if r.status_code == 200:
            logging.info('{entity_id} updated'.format(entity_id=entity_id))
        else:
            logging.info('{entity_id} update error, http_code: {code}, response: {response}'.format(entity_id=entity_id,
                                                                                                    code=r.status_code,
                                                                                                    response=r.json()))
    r.raise_for_status()


def get_auth_header(ctx, env_name, oauth, iam_token):

    if iam_token is None:
        iam_token = token_from_env("SOLOMON_IAM_TOKEN_", env_name)

    if oauth is None:
        oauth = token_from_env("SOLOMON_OAUTH_TOKEN_", env_name)
    if oauth is None:
        oauth = token_from_file('~/tokens/solomon.json', env_name)

    if iam_token:
        return 'Bearer {token}'.format(token=iam_token)
    if oauth:
        return 'OAuth {token}'.format(token=oauth)
    raise Exception("Could'n find neither IAM nor OAuth token for Solomon env %s" % env_name)


def token_from_file(token_file, env_name):
    path = Path(os.path.expanduser(token_file))
    logging.info("Token file: %s" % path)
    if path.exists() and path.is_file():
        with open(str(path)) as file_:
            content = file_.read()
        js = json.loads(content)
        if env_name in js.keys():
            return js[env_name]
    return None


def token_from_env(prefix, env_name):
    token_env = (prefix + env_name).upper()
    if token_env in os.environ.keys() and len(os.environ[token_env]) > 0:
        return os.environ[token_env]
    return None


def upload(ctx, env_name: str, kinds, oauth: str = None, iam_token: str = None, pattern: str = None):
    auth = get_auth_header(ctx, env_name, oauth, iam_token)

    headers = {
        'Content-Type': 'application/json',
        'Accept': 'application/json',
        'Authorization': auth,
    }

    assert "cfg" in ctx.obj.keys()
    assert env_name in ctx.obj["cfg"]["envs"]
    cfg = ctx.obj["cfg"]
    env = cfg["envs"][env_name]
    for entry_kind in kinds:
        update_entities(env_name,
                        env,
                        entry_kind,
                        pattern,
                        headers,
                        ctx.obj["dry_run"])
