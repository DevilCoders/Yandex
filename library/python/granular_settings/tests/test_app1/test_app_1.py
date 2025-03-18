# encoding: utf-8
import os


def test_import_settings_1():
    # Проверяем наличие особых переменных
    from .appA.settings import SETTINGS_ROOT_PATH, PROJECT_PATH, PROJECT
    print("SETTINGS_ROOT_PATH=", SETTINGS_ROOT_PATH)
    assert SETTINGS_ROOT_PATH == 'library/python/granular_settings/tests/test_app1/appA'
    print("PROJECT_PATH=", PROJECT_PATH)
    print("PROJECT=", PROJECT)
    # TODO: как-то проверять PROJECT_PATH
    # Проблема в том, что при запуске из py.test PROJECT_PATH равен чему-то вроде /usr/local/bin
