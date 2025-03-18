# coding: utf-8

from tools_structured_logs.logic.configuration.application_hooks.interface import InstrumentedApplicationHooks


class FlaskApplication(InstrumentedApplicationHooks):
    def exec_on_request_started(self, any_func):
        from flask.signals import request_started
        request_started.connect(any_func)
