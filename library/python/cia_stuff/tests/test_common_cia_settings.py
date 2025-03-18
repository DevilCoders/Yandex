# coding: utf-8

from __future__ import unicode_literals

from pretend import stub

from cia_stuff import settings as common_settings


def test_detect_aliases():
    settings = common_settings.get_settings({
        'ds': stub(
            database_host_slave='vs-mysqltools.http.yandex.net',
            database_port_slave='43306',
        )
    })

    assert settings.db_aliases == [
        'default',
        'slave',
    ]


def test_two_slaves():
    settings = common_settings.get_settings({
        'ds': stub(
            database_host_slave1='mysql01e',
            database_port_slave1='3306',
            database_host_slave2='mysql01h',
            database_port_slave3='306',
        )
    })

    assert settings.db_aliases == [
        'default',
        'slave1',
        'slave2',
    ]


def test_dont_duplicate_slaves():
    settings = common_settings.get_settings({
        'ds': stub(
            database_host_slave1='vs-mysqltools.http.yandex.net',
            database_port_slave1='43306',
            database_host_slave2='vs-mysqltools.http.yandex.net',
            database_port_slave2='43306',
        )
    })

    assert settings.db_aliases == [
        'default',
        'slave1',
    ]
