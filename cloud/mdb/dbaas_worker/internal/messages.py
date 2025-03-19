"""
Main/executor messages
"""

from .exceptions import ExposedException


class ExecutorMessage:
    """
    Message from executor to main process
    """

    def __init__(self, context):
        self.context = context


class TaskUpdate(ExecutorMessage):
    """
    Task status change message (not final)
    """

    def __init__(self, context, changes):
        super().__init__(context)
        self.changes = changes

    def __repr__(self):
        return f'TaskUpdate(changes={self.changes}, context={self.context})'


class TaskFinish(ExecutorMessage):
    """
    Task finish message (final)
    """

    def __init__(self, context, interrupted=False, rejected=False, exception=None, traceback=None, restartable=False):
        super().__init__(context)
        self.interrupted = interrupted
        self.rejected = rejected
        self.traceback = traceback
        self.restartable = restartable
        if exception is not None:
            if isinstance(exception, ExposedException):
                self.error = exception.serialize()
            else:
                self.error = ExposedException(message=repr(exception)).serialize()
        else:
            self.error = None

    def __repr__(self):
        return (
            f'TaskFinish(context={self.context}, interrupted={self.interrupted}, '
            f'rejected={self.rejected}, error={self.error}, restartable={self.restartable})'
        )
