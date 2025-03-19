"""
Noop Executor (does literally nothing)
"""
from .common.base import BaseExecutor
from .utils import register_executor


@register_executor('noop')
class NoopExecutor(BaseExecutor):
    """
    Do nothing and finish task
    """

    def run(self):
        pass
