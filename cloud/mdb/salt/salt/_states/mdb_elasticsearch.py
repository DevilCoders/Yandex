# -*- coding: utf-8 -*-
'''
ElasticSearch state for salt
'''

from __future__ import absolute_import, print_function, unicode_literals
import traceback
import json


# Initialize salt built-ins here (will be overwritten on lazy-import)
__opts__ = {}
__salt__ = {}


def ensure_plugins(name):
    """
    Ensure that all required from pillar plugins are installed.
    Also updates plugins to match current elasticsearch version.
    """
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    installed_plugins, update_plugins = __salt__['mdb_elasticsearch.installed_and_update_plugins']()
    expected_plugins = __salt__['mdb_elasticsearch.pillar_plugins']()

    # remove plugins that not in expected or require update
    to_remove = (set(installed_plugins) - set(expected_plugins)) | set(update_plugins)

    # install plugins that not in installed or require update
    to_install = set(expected_plugins) - (set(installed_plugins) - set(update_plugins))

    if not (to_install or to_remove):
        ret["result"] = True
        ret["comment"] = "All plugins already in sync."
        return ret

    # check test
    if __opts__['test']:
        ret["changes"]["plugins_install"] = list(to_install)
        ret["changes"]["plugins_remove"] = list(to_remove)
        ret['result'] = None
        return ret

    # Finally, make the actual change and return the result.
    ret["changes"]["plugins_removed"] = []
    ret["changes"]["plugins_installed"] = []
    for plugin in to_remove:
        __salt__["mdb_elasticsearch.remove_plugin"](plugin)
        ret["changes"]["plugins_removed"].append(plugin)

    for plugin in to_install:
        __salt__["mdb_elasticsearch.install_plugin"](plugin)
        ret["changes"]["plugins_installed"].append(plugin)

    ret["result"] = True

    return ret


def ensure_keystore(name, settings):
    """
    Ensure that all secure keys added to keystore.
    """
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}

    installed = __salt__['mdb_elasticsearch.keystore_keys']()
    expected = settings.keys()

    # remove plugins that not in expected
    to_remove = set(installed) - set(expected)
    to_remove.discard("keystore.seed")  # build in

    # install plugins that not in installed
    to_install = set(expected) - set(installed)

    if not (to_install or to_remove):
        ret["result"] = True
        ret["comment"] = "Keystore already in sync."
        return ret

    # check test
    if __opts__['test']:
        ret["changes"]["keystore_add"] = list(to_install)
        ret["changes"]["keystore_remove"] = list(to_remove)
        ret['result'] = None
        return ret

    # Finally, make the actual change and return the result.
    ret["changes"]["keystore_removed"] = []
    ret["changes"]["keystore_added"] = []
    for key in to_remove:
        __salt__["mdb_elasticsearch.keystore_remove"](key)
        ret["changes"]["keystore_removed"].append(key)

    for key in to_install:
        __salt__["mdb_elasticsearch.keystore_add"](key, settings[key])
        ret["changes"]["keystore_added"].append(key)

    ret["result"] = True

    return ret


def ensure_license(name, edition):
    """
    Sync Elasticsearch license
    """
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}
    if not __salt__['pillar.get']('data:elasticsearch:license:enabled', True):
        ret['result'] = True
        ret['comment'] = 'License check disabled'
        return ret

    data = __salt__['pillar.get']('data:elasticsearch:license:' + edition, '')

    installed = __salt__['mdb_elasticsearch.get_license']()['license']

    if not data:  # remove non basic license
        if installed["type"] == "basic":
            ret['result'] = True
            ret['comment'] = 'License already in sync.'
            return ret

        if __opts__['test']:
            ret['changes']['license'] = 'removed'
            ret['comment'] = 'License would be removed'
            ret['result'] = None
            return ret

        try:
            __salt__['mdb_elasticsearch.delete_license']()

            ret['changes']['license'] = 'removed'
            ret['comment'] = 'License was removed'
            ret['result'] = True
            return ret
        except Exception:
            ret['result'] = False
            ret['comment'] = traceback.format_exc()
            return ret

    expected = json.loads(data)['license']

    if expected['uid'] == installed['uid']:
        ret['result'] = True
        ret['comment'] = 'License already in sync.'
        return ret

    if __opts__['test']:
        ret['changes']['license'] = 'updated'
        ret['comment'] = 'License would be updated'
        ret['result'] = None
        return ret

    try:
        __salt__['mdb_elasticsearch.update_license'](data)

        ret['changes']['license'] = 'updated'
        ret['comment'] = 'License was updated'
        ret['result'] = True
        return ret
    except Exception:
        ret['result'] = False
        ret['comment'] = traceback.format_exc()
        return ret


def ensure_repository(name, reponame, settings):
    """
    Sync Elasticsearch repository settings
    """
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}
    if not __salt__['mdb_elasticsearch.auto_backups_enabled']():
        ret['result'] = True
        ret['comment'] = 'Repositories check disabled'
        return ret

    existing, ok = __salt__['mdb_elasticsearch.get_repository'](reponame)

    existing_settings = existing[reponame]['settings'] if ok else None
    if ok and existing_settings == settings:
        ret['result'] = True
        ret['comment'] = 'Repository {repo} already in sync.'.format(repo=reponame)
        return ret

    if __opts__['test']:
        ret['changes'][reponame] = {
            'old_settings': existing_settings,
            'new_settings': settings,
        }
        ret['comment'] = 'Repository would be updated'
        ret['result'] = None
        return ret

    try:
        __salt__['mdb_elasticsearch.update_repository'](reponame, settings)

        ret['changes'][reponame] = {
            'old_settings': existing_settings,
            'new_settings': settings,
        }
        ret['comment'] = 'Repository was updated'
        ret['result'] = True
        return ret
    except Exception:
        ret['result'] = False
        ret['comment'] = traceback.format_exc()
        return ret


def ensure_snapshot_policy(name, policy_name, settings):
    """
    Sync Elasticsearch repository settings
    """
    ret = {'name': name, 'changes': {}, 'result': False, 'comment': ''}
    if not __salt__['mdb_elasticsearch.auto_backups_enabled']():
        ret['result'] = True
        ret['comment'] = 'Repositories(snapshots) check disabled'
        return ret

    existing, ok = __salt__['mdb_elasticsearch.get_snapshot_policy'](policy_name)

    existing_policy = existing[policy_name]['policy'] if ok else None
    if ok and existing_policy == settings:
        ret['result'] = True
        ret['comment'] = 'Snapshot policy {policy} already in sync.'.format(policy=policy_name)
        return ret

    if __opts__['test']:
        ret['changes'][policy_name] = {
            'old_policy': existing_policy,
            'new_policy': settings,
        }
        ret['comment'] = 'Snapshot policy would be updated'
        ret['result'] = None
        return ret

    try:
        __salt__['mdb_elasticsearch.update_snapshot_policy'](policy_name, settings)

        ret['changes'][policy_name] = {
            'old_settings': existing_policy,
            'new_settings': settings,
        }
        ret['comment'] = 'Snapshot policy was updated'
        ret['result'] = True
        return ret
    except Exception:
        ret['result'] = False
        ret['comment'] = traceback.format_exc()
        return ret
