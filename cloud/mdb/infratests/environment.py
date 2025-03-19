"""
Behave entry point.
"""
import logging

from cloud.mdb.infratests.config import build_config
from cloud.mdb.infratests.test_helpers.context import Context
from cloud.mdb.infratests.test_helpers.iam import get_iam_token

# todo
# * receive part of config from terraform
# * rename test_helpers -> helpers ?
# * setup context.logger


def before_all(context: Context):
    """
    Prepare environment for tests.
    """

    context.logger = logging.getLogger()
    # behave's Context has its own config field so we have to use another name
    context.test_config = build_config()
    context.user_iam_token = get_iam_token(context.test_config)


def before_scenario(context, _):
    """
    Cleanup function executing per feature scenario.
    """
    pass


def before_feature(context, _):
    """
    Cleanup function executing per feature.
    """
    pass


def after_step(context, step):
    """
    Save logs after failed step
    """
    pass


def after_all(context):
    """
    Clean up and collect info.
    """
