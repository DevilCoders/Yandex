"""
Behave entry point
"""

import random
import signal
import string
import logging

from configuration import get_configuration
from helpers.yandexcloud import setup_environment, teardown_environment, is_disabled_cleanup
from helpers import dataproc, tunnel

logging.basicConfig(
    format='%(asctime)s {%(name)s} [%(levelname)s]:\t%(message)s',
    level=logging.INFO,
)


def before_all(ctx):
    """
    Prepare environment for tests
    """
    # Accident teardown
    signal.signal(signal.SIGINT, lambda *args: teardown_environment(ctx))
    signal.signal(signal.SIGTERM, lambda *args: teardown_environment(ctx))

    ctx.run_id = ''.join(random.choice(string.ascii_lowercase) for i in range(8))
    ctx.id = f'dp-{ctx.run_id}'
    ctx.conf = get_configuration()
    setup_environment(ctx)


def before_scenario(ctx, _):
    """
    Cleanup function executing per feature scenario
    """
    ctx.state['initialization_actions'] = []


def before_feature(ctx, _):
    """
    Cleanup function executing before feature
    """
    ctx.state['topology'] = (1, 1, 1)
    ctx.state['isolate_network'] = True
    ctx.state['properties'] = dataproc.default_properties(ctx)
    ctx.state['command'] = None
    ctx.state['initialization_actions'] = []


def after_feature(ctx, feature):
    """
    Cleanup function executing per feature
    """
    if not is_disabled_cleanup():
        dataproc.dataproc_clean(ctx)
    tunnel.tunnels_clean(ctx)


def after_step(ctx, step):
    """
    Save diagnostic after failed step
    """
    if step.status == 'failed':
        if ctx.conf.get('save_diagnostics', False):
            dataproc.save_diagnostics(ctx)


def after_all(ctx):
    """
    Cleanup
    """
    teardown_environment(ctx)
