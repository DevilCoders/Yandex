# encoding: utf-8
from __future__ import print_function
import inspect
import os
import typing as tp  # noqa

import yatest

from library.python.testing.types_test.py2 import MypyTyping
from library.python.testing.types_test.py2.config import DefaultMyPyConfig


def get_conftest_testmodule():
    # type: () -> str
    """ `mypy_check_module()` should be a function defined in user's confest test file """
    import conftest
    assert hasattr(conftest, 'mypy_check_module'), "No mypy_check_module() function defined in conftest module"
    return conftest.mypy_check_module()


def get_conftest_badmodules():
    # type: () -> tp.List[str]
    """ if `modules_to_delete` (a list with modules to delete) defined in user's confest test file then delete them"""
    import conftest
    if hasattr(conftest, 'modules_to_delete'):
        return conftest.modules_to_delete
    return []


def get_conftest_recursive_param():
    # type: () -> bool
    import conftest
    if hasattr(conftest, 'check_recursive'):
        return conftest.check_recursive
    return False


def get_conftest_mypy_config():
    # type: () -> tp.List[str]
    """ if configuration `class MyPyConfig` defined in user's confest test file then load"""
    import conftest
    if hasattr(conftest, 'MyPyConfig'):
        assert isinstance(conftest.MyPyConfig, type) and issubclass(conftest.MyPyConfig, DefaultMyPyConfig), "MyPyConfig() is not inherited from DefaultMyPyConfig"
        return {k: v for k, v in inspect.getmembers(conftest.MyPyConfig, lambda a: not(inspect.isroutine(a))) if not k.startswith('_')}
    return {k: v for k, v in DefaultMyPyConfig.__dict__.items() if not k.startswith('_')}


def test_module(links):
    m = MypyTyping("extraction_mypy_typing")
    module = get_conftest_testmodule()
    bad_modules = get_conftest_badmodules()
    mypy_config = get_conftest_mypy_config()
    check_recursive = get_conftest_recursive_param()
    try:
        m.start_scan(module, recursive=check_recursive, bad_modules=bad_modules, **mypy_config)
    finally:
        path = yatest.common.output_path("reports")
        if os.path.exists(path):
            links.set("mypy reports", path)
