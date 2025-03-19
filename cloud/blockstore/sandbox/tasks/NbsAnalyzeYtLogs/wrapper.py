class Wrapper(object):

    def __init__(self, task):
        self._task = task

    @property
    def task(self):
        return self._task

    @property
    def parameters(self):
        return self.task.Parameters

    @property
    def context(self):
        return self.task.Context

    @property
    def yt_oauth(self):
        return self.parameters.yt_oauth

    @property
    def yt_pool(self):
        return self.parameters.yt_pool
