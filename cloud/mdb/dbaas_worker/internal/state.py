"""
Executor state
"""


class ExecutorState:
    """
    Task executor state
    """

    def __init__(
        self,
        changes,
        comment,
        context,
        updated=False,
        result=None,
        errors=None,
        interrupted=False,
        rejected=False,
        restartable=False,
    ):
        self.changes = changes
        self.comment = comment
        self.context = context
        self.updated = updated
        self.result = result
        self.errors = errors
        self.interrupted = interrupted
        self.rejected = rejected
        self.restartable = restartable
