class ExecutionStatus(object):
    waiting = 'waiting'
    running = 'running'
    completed = 'completed'
    undefined = 'undefined'


class ExecutionResult(object):
    cancel = 'cancel'
    success = 'success'
    failure = 'failure'
    undefined = 'undefined'
