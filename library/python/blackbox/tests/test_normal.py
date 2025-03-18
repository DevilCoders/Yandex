# -*- coding: utf-8 -*-
from __future__ import unicode_literals
"""Несколько тестов на нормальное поведение blackbox.

Должны запускаться с машины, у которой есть доступ к разработческому серверу чёрного
ящика
"""

import unittest
import pytest

import blackbox as bb

bb.BLACKBOX_URL = bb.BLACKBOX_URL_DEVELOPMENT


class TestBlackbox(unittest.TestCase):
    @pytest.mark.skip(reason="no way of currently testing this")
    def test_login(self):
        # Не проходит, так как судя по всему не осталось машин на которых есть гранты на login
        r = bb.login(
            'python-blackbox-test', 'crocodiles',
            '8.8.8.8', dbfields=[bb.FIELD_FIO, ('account_info.sex.uid', 'ttt')]
        )
        self.assertEqual(r.status, 'VALID')
        self.assertEqual(r.error, 'OK')
        self.assertEqual(r.uid, '124624990')
        self.assertEqual(r.fields.fio, 'Pupkin Vasily')
        self.assertEqual(r.fields.ttt, '1')

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_userinfo(self):
        r = bb.userinfo(
            '124624990', '8.8.8.8', dbfields=[bb.FIELD_LOGIN]
        )
        self.assertEqual(r.fields.login, 'python-blackbox-test')
        self.assertEqual(r.fields.display_name, 'python-blackbox-test')
        self.assertEqual(r.fields.social, None)
        self.assertEqual(r.fields.social_aliases, None)

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_social(self):
        # tw: python_blackbox:crocodiles
        r = bb.userinfo(
            '124246245', '8.8.8.8', dbfields=[bb.FIELD_LOGIN]
        )
        self.assertEqual(r.fields.login, 'uid-zsj4mpx4')
        self.assertEqual(r.fields.social.provider, 'tw')
        self.assertEqual(r.fields.social.profile_id, '270705')
        self.assertEqual(r.fields.display_name, 'Вася Пупкин')

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_oauth(self):
        r = bb.oauth(
            'AQAAAAAHbaBeAAGrlqF810b0UkHyszyQx7ckTlc', '8.8.8.8', by_token=True,
            dbfields=[bb.FIELD_LOGIN]
        )
        self.assertEqual(r.fields.login, 'python-blackbox-test')
        self.assertEqual(r.oauth.client_id, '2c1ab9ef91cd459ca3670a57f7f981f0')
        self.assertEqual(r.oauth.client_name, 'Тестовое приложение для тестов')

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_uid(self):
        r = bb.uid('python-blackbox-test')
        self.assertEqual(r, 124624990)

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_subscription(self):
        r = bb.subscription(124624990, 5)      # Фотки
        self.assert_(r is None)

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_subscription_fake_uid(self):
        # не существующий пользователь 678679991111
        # вываливлась ошибка KeyError: 'suid'

        r = bb.subscription(678679991111, 23)      # Подписки
        self.assert_(r is None)

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_sex(self):
        self.assertEqual(bb.sex(124624990), 1)

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_lite(self):
        self.assertFalse(bb.is_lite(124624990))

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_country(self):
        self.assertEqual(bb.country(124624990), 'ru')

    @pytest.mark.skip(reason="no way of currently testing this")
    def test_country_fake_uid(self):
        self.assert_(bb.country(12462499011111111) is None)
