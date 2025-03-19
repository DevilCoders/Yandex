"""
Providers common. BaseProvider is defined here.
"""

import json
from datetime import datetime

from copy import deepcopy

from ..exceptions import Interrupted
from ..logs import get_task_prefixed_logger
from ..messages import TaskUpdate


class Change:
    """
    Provider change
    """

    def __init__(self, key, value, context=None, rollback=None, critical=False):
        self.key = key
        self.value = value
        self.timestamp = datetime.utcnow()
        self.context = context if context else dict()
        if critical and not rollback:
            raise RuntimeError('Critical change without rollback path')
        self.critical = critical
        self._rollback = rollback

    def serialize(self):
        """
        Return database representation of change
        """
        return {
            self.key: self.value,
            'timestamp': str(self.timestamp),
        }

    def can_rollback(self):
        """
        Check if rollback is defined for this change
        """
        return self._rollback is not None

    def rollback(self, task, safe_revision):
        """
        Rollback individual change
        """
        return self._rollback(task, safe_revision)

    def __str__(self):
        return json.dumps(self.serialize())

    def __repr__(self):
        return f'Change({json.dumps(self.serialize())})'

    @staticmethod
    def noop_rollback(task, save_revision):
        pass


# pylint: disable=too-few-public-methods
class InterruptContext:
    """
    Interrupt context helper
    """

    def __init__(self, task, queue):
        self.task = task
        self.queue = queue

    def __enter__(self):
        self.task['context']['interruptable'] = True
        self.queue.put(TaskUpdate({'interruptable': True}, None))
        if self.task['context'].get('interrupted'):
            raise Interrupted('Interrupted by external signal')

    def __exit__(self, *_):
        self.task['context']['interruptable'] = False
        self.queue.put(TaskUpdate({'interruptable': False}, None))


# pylint: disable=too-few-public-methods
class BaseProvider:
    """
    Base provider class
    """

    def __init__(self, config, task, queue):
        self.config = config
        self.queue = queue
        self.task = task
        self.logger = get_task_prefixed_logger(task, __name__)
        self.interruptable = InterruptContext(task, queue)

    def add_change(self, change):
        """
        Save change
        """
        self.task['changes'].append(change)
        self.task['context'].update(change.context)
        if change.context:
            self.logger.debug('Saving context: %s', change.context)
        serialized = change.serialize()
        self.queue.put(TaskUpdate(change.context, serialized))

    def context_get(self, key):
        """
        Task context get helper
        """
        value = self.task['context'].get(key)
        self.logger.debug('Getting %s from context: %s', key, value)
        return deepcopy(value)
