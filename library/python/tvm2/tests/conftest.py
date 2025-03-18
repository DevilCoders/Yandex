# coding: utf-8

from __future__ import unicode_literals

import os

import pytest
import vcr

from mock import patch
from tvmauth import BlackboxTvmId as BlackboxClientId
from tvmauth.mock import TvmClientPatcher, MockedTvmClient


@pytest.fixture
def test_vcr():
    try:
        import yatest
        path = yatest.common.source_path('library/python/tvm2/vcr_cassettes')
    except ImportError:
        path = os.path.abspath(os.path.join(os.curdir, 'vcr_cassettes'))
    return vcr.VCR(
        cassette_library_dir=path,
    )


@pytest.fixture
def threaded_tvm(test_vcr):
    # Импортируем локально, чтобы не влиять на тест на импорты
    from tvm2.sync.thread_tvm2 import TVM2
    TVM2._instance = None
    with TvmClientPatcher(MockedTvmClient(self_tvm_id=28)):
        return TVM2(
            client_id='28',
            secret='GRMJrKnj4fOVnvOqe-WyD1',
            blackbox_client=BlackboxClientId.Test,
            destinations=['28'],
            retries=0,
        )


@pytest.fixture
def tvmqloud(test_vcr, monkeypatch):
    from tvm2.sync.daemon_tvm2 import TVM2Daemon
    TVM2Daemon._instance = None
    monkeypatch.setenv('QLOUD_TVM_TOKEN', 'test_token')
    return TVM2Daemon(
        client_id='28',
        blackbox_client=BlackboxClientId.Test,
        retries=0,
    )


@pytest.fixture
def tvmdeploy(test_vcr, monkeypatch):
    from tvm2.sync.daemon_tvm2 import TVM2Daemon
    TVM2Daemon._instance = None
    monkeypatch.setenv('TVMTOOL_LOCAL_AUTHTOKEN', 'deploy_test_token')
    monkeypatch.setenv('DEPLOY_TVM_TOOL_URL', 'http://localhost:100500')
    return TVM2Daemon(
        client_id='28',
        blackbox_client=BlackboxClientId.Test,
        retries=0,
    )


@pytest.fixture
def valid_service_ticket():
    return (
        '3:serv:CBAQ__________9_IhkI5QEQHBoIYmI6c2VzczEaCGJiOnNlc3My:WUP'
        'x1cTf05fjD1exB35T5j2DCHWH1YaLJon_a4rN-D7JfXHK1Ai4wM4uSfboHD9xmG'
        'QH7extqtlEk1tCTCGm5qbRVloJwWzCZBXo3zKX6i1oBYP_89WcjCNPVe1e8jwGdL'
        'snu6PpxL5cn0xCksiStILH5UmDR6xfkJdnmMG94o8'
    )


@pytest.fixture
def invalid_service_ticket():
    return (
        '3:serv:CBAQ__________9_czEaCGJiOnNlc3My:WUPx1cTf05fjD1exB35T5j2DCH'
        'WH1YaLJon_a4rN-D7JfXHK1Ai4wM4uSfboHD9xmGQH7extqtlEk1tCTCGm5qbRVloJ'
        'wWzCZBXo3zKX6i1oBYP_89WcjCNPVe1e8jwGdLsnu6PpxL5cn0xCksiStILH5U'
        'mDR6xfkJdnmMG94o8'
    )


@pytest.fixture
def valid_user_ticket():
    return (
        '3:user:CA0Q__________9_GiQKAwjIAwoCCHsQyAMaCGJiOnNlc3MxGghiYjpzZX'
        'NzMiASKAE:KJFv5EcXn9krYk19LCvlFrhMW-R4q8mKfXJXCd-RBVBgUQzCOR1Dx2F'
        'iOyU-BxUoIsaU0PiwTjbVY5I2onJDilge70Cl5zEPI9pfab2qwklACq_ZBUvD1tzrf'
        'NUr88otBGAziHASJWgyVDkhyQ3p7YbN38qpb0vGQrYNxlk4e2I'
    )


@pytest.fixture
def invalid_user_ticket():
    return (
        '3:user:CA0Q__________9_GiQKAwjIAwoCCHsQyAMaCGJiOnNlc3MxcXn9krYk19LCv'
        'lFrhMW-R4q8mKfXJXCd-RBVBgUQzCOR1Dx2FiOyU-BxUoIsaU0PiwTjbVY5I2onJDilge'
        '70Cl5zEPI9pfab2qwklACq_ZBUvD1tzrfNUr88otBGAziHASJWgyVDkhy'
        'Q3p7YbN38qpb0vGQrYNxlk4e2I'
    )
