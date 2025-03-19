import django.utils.safestring as dus

import metrika.pylib.structures.dotdict as mdd


class Response(mdd.DotDict):
    def __init__(self):
        self.errors = []
        self.messages = []
        self.data = mdd.DotDict()
        self._meta = {}
        self.html = ''
        self.result = True
        self.valid = True
        self.html_title = None
        self.label = None

    def add_msg(self, message, safe=False):
        if message:
            if safe:
                message = dus.mark_safe(message)
            self.messages.append(message)

    def add_err(self, error, safe=False):
        if error:
            if safe:
                error = dus.mark_safe(error)
            self.errors.append(error)

    def mark_failed(self, error, safe=False):
        self.add_err(error, safe=safe)
        self.result = False

    def mark_success(self, message, safe=False):
        self.add_msg(message, safe=safe)
        self.result = True

    def parse_result(self, result, reason):
        if not result:
            self.mark_failed(reason)
        else:
            self.mark_success(reason)
