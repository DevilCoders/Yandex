# coding: utf-8

import six

if six.PY3:
    from importlib import import_module

from .session_class_wrapper import wrap_session_send
from tools_structured_logs.logic.log_records.vendors.base_vendor import IVendorInstrumenter
from ..info_provider import RequestsLibDomainProvider
from tools_structured_logs.logic.configuration.library import get_config


def dynamic_import_py2(abs_module_path, class_name):
    module_object = __import__(abs_module_path, fromlist=[class_name])
    return getattr(module_object, class_name)


def dynamic_import_py3(abs_module_path, class_name):
    module_object = import_module(abs_module_path)
    return getattr(module_object, class_name)


def dynamic_import(abs_module_path, class_name):
    try:
        if six.PY2:
            return dynamic_import_py2(abs_module_path, class_name)
        elif six.PY3:
            return dynamic_import_py3(abs_module_path, class_name)
    except (ImportError, AttributeError):
        return None


class SessionClassInstrumenter(IVendorInstrumenter):

    @property
    def import_map(self):
        return {
            module_path: None
            for module_path in get_config().get_requests_modules_to_patch()
        }

    def instrument(self):
        for module_path, send_ref in self.import_map.items():
            if send_ref is None:
                session_class = dynamic_import(module_path, 'Session')
                if session_class is not None:
                    self.wrap_session(session_class)

    def wrap_session(self, session_class):
        session_send = getattr(session_class, 'send')
        setattr(session_class, 'send', wrap_session_send(session_send, RequestsLibDomainProvider()))
