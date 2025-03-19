"""
Module providing API for registration and cleanup of side-effects.
"""
from collections import defaultdict
from functools import wraps

import requests
from retrying import retry

from tests.helpers import compose
from tests.helpers.fake_iam import get_access_service_url
from tests.helpers.mock import MockClient

_DATA = {
    'scenario': {
        'projects': set(),
        'cleanup_strategies': dict(),
    },
    'feature': {
        'projects': set(),
        'cleanup_strategies': dict(),
    },
}


def side_effects(*, scope=None, project=None, projects=None):
    """
    Decorator registering side-effects produced by a test step.

    The parameters "project" and "projects" specify one or several projects
    affecting by the test step.
    """
    assert scope is not None, \
        'scope must be set'

    def decorator(fun):
        @wraps(fun)
        def wrapper(*args, **kwargs):
            if project:
                _DATA[scope]['projects'].add(project)
            if projects:
                _DATA[scope]['projects'].update(projects)
            return fun(*args, **kwargs)

        return wrapper

    return decorator


def cleanup_side_effects(context, scope='scenario'):
    """
    Clean up registered side-effects.
    """
    project_map = defaultdict(set)

    for project in _DATA[scope]['projects']:
        config = context.conf['projects'][project]
        cleanup_strat = config.get('cleanup_strategy', 'restart')
        project_map[cleanup_strat].add(project)

    for strategy, projects in project_map.items():
        _DATA[scope]['cleanup_strategies'][strategy](context, projects)

    _DATA[scope]['projects'].clear()


def cleanup_strategy(scope, name):
    """
    Decorator registering a cleanup strategy.
    """

    def decorator(func):
        _DATA[scope]['cleanup_strategies'][name] = func
        return func

    return decorator


@cleanup_strategy('scenario', 'restart')
@cleanup_strategy('feature', 'restart')
def _restart(context, projects):
    compose.restart_containers(context.conf, projects=projects)


@cleanup_strategy('scenario', 'recreate')
@cleanup_strategy('feature', 'recreate')
def _recreate(context, projects):
    compose.recreate_containers(context.conf, projects=projects)


@cleanup_strategy('scenario', 'reset_mock')
@cleanup_strategy('feature', 'reset_mock')
def _reset_mock(context, projects):
    for project in projects:
        MockClient(context, project).reset()


@cleanup_strategy('scenario', 'reset_dbm_mock')
@cleanup_strategy('feature', 'reset_dbm_mock')
def _reset_dbm_mock(context, projects):
    compose.remove_orphans(context.conf)
    _reset_mock(context, projects)


@cleanup_strategy('scenario', 'reset_access_service_mock')
@retry(wait_fixed=100, stop_max_attempt_number=600)
def reset_access_service_mock(context, *_):
    """
    Set access service mock configuration to default
    """
    url = get_access_service_url(context)
    for i in ['authorize', 'authenticate']:
        requests.put(
            '{url}/{type}'.format(url=url, type=i),
            json={
                'user_account': {
                    'id': 'test-user',
                },
            },
        ).raise_for_status()
