# -*- coding: utf-8 -*-
"""
Shell within infratests env
"""

from tests import env_control
from cloud.mdb.internal.python.ipython_repl import UserNamespace, start_repl


def infratest_shell():
    """
    Shell interactive entry-point
    """

    state = env_control._init_state()
    config = state['config']
    del state['config']

    start_repl(
        [
            UserNamespace(state, 'state'),
            UserNamespace(config, 'config'),
        ]
    )
