#!/usr/bin/env python
import unittest
import os
from contextlib import contextmanager

import six
import yenv  # noqa


def reload_yenv():
    global yenv
    yenv = six.moves.reload_module(yenv)


@contextmanager
def env(**kwargs):
    old = os.environ.copy()

    for name, value in six.iteritems(kwargs):
        os.environ[name] = value

    reload_yenv()

    yield

    os.environ.clear()
    os.environ.update(old)


class TestCase(unittest.TestCase):
    def test_env_valiable(self):
        with env(YENV_TYPE='test-type'):
            self.assertEqual(yenv.type, 'test-type')

        with env(YENV_NAME='test-name'):
            self.assertEqual(yenv.name, 'test-name')

    def test_chain(self):
        with env(YENV_TYPE='test.suffix'):
            result = list(yenv.chain_type())

            self.assertEqual(result, ['test.suffix', 'test'])

        with env(YENV_TYPE='test'):
            result = list(yenv.chain_type())

            self.assertEqual(result, ['test'])

    def test_choose(self):
        with env(YENV_TYPE='test.suffix'):
            result = yenv.choose_type(['foo', 'test.suffix', 'bar'])

            self.assertEqual(result, 'test.suffix')

        with env(YENV_TYPE='test.suffix'):
            result = yenv.choose_type(['foo', 'test', 'bar'])

            self.assertEqual(result, 'test')

        with env(YENV_TYPE='test'):
            self.assertRaises(ValueError, yenv.choose_type, ['foo', 'bar'])

        with env(YENV_TYPE='test.1'):
            self.assertRaises(ValueError, yenv.choose_type, ['foo', 'test.2', 'bar'])

    def test_fallback(self):
        with env(YENV_TYPE='development'):
            result = yenv.choose_type(['testing', 'production'], fallback=True)

            self.assertEqual(result, 'testing')

        with env(YENV_TYPE='prestable'):
            result = yenv.choose_type(['testing', 'production'], fallback=True)

            self.assertEqual(result, 'production')

    def test_choose_key(self):
        mapping = {
            'foo': 1,
            'test': 2,
            'bar': 3,
        }

        with env(YENV_TYPE='test.suffix'):
            result = yenv.choose_key_by_type(mapping)

            self.assertEqual(result, 2)

    def test_choose_key_with_fallback(self):
        mapping = {
            'development': 'localhost',
            'testing': 'service.test.yandex-team.ru',
            'production': 'service.yandex-team.ru',
        }

        expected = {
            'development': 'localhost',
            'unstable': 'service.test.yandex-team.ru',
            'testing': 'service.test.yandex-team.ru',
            'prestable': 'service.yandex-team.ru',
            'production': 'service.yandex-team.ru',
        }

        for case_environment, expected_value in expected.items():
            with env(YENV_TYPE=case_environment):
                result = yenv.choose(mapping)
            self.assertEqual(result, expected_value)

    def test_choose_kw(self):
        mapping = {
            'development': 'localhost',
            'testing': 'service.test.yandex-team.ru',
            'production': 'service.yandex-team.ru',
        }

        expected = {
            'development': 'localhost',
            'unstable': 'service.test.yandex-team.ru',
            'testing': 'service.test.yandex-team.ru',
            'prestable': 'service.yandex-team.ru',
            'production': 'service.yandex-team.ru',
        }

        for case_environment, expected_value in expected.items():
            with env(YENV_TYPE=case_environment):
                result = yenv.choose_kw(**mapping)
            self.assertEqual(result, expected_value)


if __name__ == '__main__':
    unittest.main()
