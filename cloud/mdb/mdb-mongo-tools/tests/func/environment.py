"""
Behave entry point.

For details of env bootstrap, see env_control
"""

import logging

from tests.func import env_control


class SafeStorage:
    """
    Test data storage
    """

    # pylint: disable=missing-docstring

    def __init__(self):
        self._mongodb_hosts = []
        self._mongodb_pids = {}
        self._mongodb_test_data = {}

    @property
    def mongodb_hosts(self):
        assert self._mongodb_hosts, 'Accessing empty item'
        return self._mongodb_hosts

    @mongodb_hosts.setter
    def mongodb_hosts(self, item):
        assert isinstance(item, list)
        self._mongodb_hosts = item

    @property
    def mongodb_pids(self):
        assert self._mongodb_pids, 'Accessing empty item'
        return self._mongodb_pids

    @mongodb_pids.setter
    def mongodb_pids(self, item):
        assert isinstance(item, dict)
        self._mongodb_pids = item

    @property
    def mongodb_test_data(self):
        return self._mongodb_test_data

    @mongodb_test_data.setter
    def mongodb_test_data(self, item):
        assert isinstance(item, dict)
        self._mongodb_test_data = item


storage = SafeStorage()


def before_all(context):
    """
    Prepare environment for tests.
    """
    context.state = env_control.create()
    context.conf = context.state['config']
    context.config.stop = True


def before_feature(context, _feature):
    """
    Cleanup function executing per feature.
    """
    env_control.restart(state=context.state)


def before_scenario(context, _):
    """
    Cleanup function executing per scenario.
    """
    env_control.restart(state=context.state)


def before_step(context, _):
    """
    Launch debug before step
    """
    context.storage = storage


def after_all(context):
    """
    Clean up.
    """
    if context.failed and not context.aborted and not context.conf.get('cleanup'):
        logging.warning('Remember to run `make clean` after you done')
        return
    if hasattr(context, 'state'):
        env_control.stop(state=context.state)


def after_step(context, step):
    """
    Save logs after failed step
    """
    if step.status == 'failed':
        if context.config.userdata.getbool('debug'):
            import pdb
            pdb.post_mortem(step.exc_traceback)
