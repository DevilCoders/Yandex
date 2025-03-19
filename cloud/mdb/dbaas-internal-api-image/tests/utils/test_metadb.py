"""
MetaDB utils tests
"""

import unittest.mock

import pytest

from dbaas_internal_api.utils.metadb import commit_on_success

FLASK_G = 'dbaas_internal_api.utils.metadb.g'


def test_commit_on_success_do_commit_if_no_exception_raised(mocker, monkeypatch):
    flask_g = mocker.Mock()
    monkeypatch.setattr(FLASK_G, flask_g)

    with commit_on_success():
        pass

    assert flask_g.mock_calls == [unittest.mock.call.metadb.commit()]


def test_commit_on_success_do_not_commit_on_exceptions(mocker, monkeypatch):
    flask_g = mocker.Mock()
    monkeypatch.setattr(FLASK_G, flask_g)

    with pytest.raises(RuntimeError):
        with commit_on_success():
            raise RuntimeError('Something bad happens')

    assert flask_g.mock_calls == []
