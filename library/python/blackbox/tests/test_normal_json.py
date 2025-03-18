# -*- coding: utf-8 -*-
from __future__ import unicode_literals
"""Несколько тестов на нормальное поведение JsonBlackbox.

Должны запускаться с машины, у которой есть доступ к разработческому серверу
чёрного ящика
"""

import unittest
import pytest

import blackbox

jb = blackbox.JsonBlackbox(blackbox.BLACKBOX_URL_DEVELOPMENT,
                           blackbox.HTTP_TIMEOUT)


class TestJsonBlackbox(unittest.TestCase):

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_login(self):
        # Не проходит, так как судя по всему не осталось машин на которых есть гранты на login
        r = jb.login(
            'python-blackbox-test', 'crocodiles', '8.8.8.8',
            dbfields=['account_info.fio.uid', 'account_info.sex.uid']
        )
        self.assertEqual(r['status']['value'], 'VALID')
        self.assertEqual(r['error'], 'OK')
        self.assertEqual(r['uid']['value'], '124624990')
        self.assertEqual(r['dbfields']['account_info.fio.uid'], 'Pupkin Vasily')
        self.assertEqual(r['dbfields']['account_info.sex.uid'], '1')

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_userinfo(self):
        r = jb.userinfo(uid='124624990', userip='8.8.8.8',
                        dbfields=['accounts.login.uid'],
                        regname='yes', aliases='getsocial')

        self.assertEqual(r['users'][0]['dbfields']['accounts.login.uid'], 'python-blackbox-test')
        self.assertEqual(r['users'][0]['display_name']['name'], 'python-blackbox-test')
        self.assertEqual(r['users'][0]['display_name'].get('social'), None)
        self.assertEqual(r['users'][0]['aliases'], {})

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_social(self):
        # tw: python_blackbox:crocodiles
        r = jb.userinfo(uid='124246245', userip='8.8.8.8',
                        dbfields=['accounts.login.uid'],
                        regname='yes', aliases='getsocial')

        self.assertEqual(r['users'][0]['dbfields']['accounts.login.uid'], 'uid-zsj4mpx4')
        self.assertEqual(r['users'][0]['display_name']['social']['provider'], 'tw')
        self.assertEqual(r['users'][0]['display_name']['social']['profile_id'], '270705')
        self.assertEqual(r['users'][0]['display_name']['name'], 'Вася Пупкин')

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_oauth(self):
        r = jb.oauth(oauth_token='AQAAAAAHbaBeAAGrlqF810b0UkHyszyQx7ckTlc',
                     userip='8.8.8.8', dbfields=['accounts.login.uid'])
        self.assertEqual(r['dbfields']['accounts.login.uid'], 'python-blackbox-test')
        self.assertEqual(r['oauth']['client_id'], '2c1ab9ef91cd459ca3670a57f7f981f0')
        self.assertEqual(r['oauth']['client_name'], 'Тестовое приложение для тестов')
