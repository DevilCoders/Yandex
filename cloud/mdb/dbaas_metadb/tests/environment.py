"""
Behave environment file
"""

import logging


def after_step(context, step):
    """
    Run debugger if user calls us with debug

    behave tests/features/host.feature -D debug
    """
    if step.status == 'failed':
        if context.config.userdata.getbool('debug'):
            try:
                import ipdb as pdb
            except ImportError:
                import pdb
            pdb.post_mortem(step.exc_traceback)


def after_scenario(context, scenario):
    """
    Close open connections
    """
    # pylint: disable=unused-argument
    if 'trans' in context:
        logging.warning('Looks like there are open transaction %r after %r', context.trans, scenario)
        context.trans.close()
