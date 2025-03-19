"""
Behave entry point.

For details of env bootstrap, see env_control
"""
import logging
import signal
from traceback import print_exc

import requests

from tests import env_control, logs
from tests.helpers.side_effects import (cleanup_side_effects, reset_access_service_mock)


def before_all(context):
    """
    Prepare environment for tests.
    """
    requests.packages.urllib3.disable_warnings()  # pylint: disable=no-member

    context.state = env_control.create()
    env_control.restart(state=context.state)
    context.conf = context.state['config']
    if context.conf.get('fail_fast', False):
        context.config.stop = True
    if not context.conf.get('disable_cleanup', False):
        signal.signal(signal.SIGINT, lambda *args: env_control.stop(state=context.state))
        signal.signal(signal.SIGTERM, lambda *args: env_control.stop(state=context.state))


def before_scenario(context, _):
    """
    Cleanup function executing per feature scenario.
    """
    cleanup_side_effects(context, scope='scenario')


def before_feature(context, _):
    """
    Cleanup function executing per feature.
    """
    reset_access_service_mock(context)
    cleanup_side_effects(context, scope='feature')


def after_step(context, step):
    """
    Save logs after failed step
    """
    if step.status == 'failed':
        try:
            logs.save_logs(context)
        except Exception:
            print('Unable to save logs')
            print_exc()

        if context.config.userdata.getbool('debug'):
            try:
                import ipdb as pdb  # pylint: disable=import-outside-toplevel
            except ImportError:
                import pdb  # pylint: disable=import-outside-toplevel
            pdb.post_mortem(step.exc_traceback)

        if context.config.userdata.getbool('skip-dependent-scenarios'):
            if 'dependent-scenarios' in context.feature.tags:
                for scenario in context.feature.scenarios:
                    if scenario.status == 'untested':
                        scenario.skip('Skip due "%s" fail' % context.scenario.name)


def after_all(context):
    """
    Clean up.
    """
    if context.failed and not context.aborted \
            and context.conf.get('disable_cleanup', False):
        logging.warning('Remember to run `make clean` after you done')
        return
    env_control.stop(state=context.state)


def _check_tags(context, scenario):
    tags = list(filter(lambda tag: tag.startswith('require_version_'), scenario.tags))
    assert len(tags) <= 1, "Only one require_version_X_Y accepted"
    if len(tags) == 1:
        req_ver_parts = tags[0][len("require_version_"):].split('.')
        assert len(req_ver_parts) == 2, "Invalid required version"
        maj_req_ver, min_req_ver = int(req_ver_parts[0]), int(req_ver_parts[1])

        version_parts = context.cluster['clickhouse']['version'].split('.')

        maj_ver, min_ver = int(version_parts.get(0, 0)), int(version_parts.get(1, 0))

        if maj_ver < maj_req_ver or (maj_ver == maj_req_ver and min_ver < min_req_ver):
            scenario.mark_skipped()
