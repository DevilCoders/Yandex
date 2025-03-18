import pytest

from tvmauth import BlackboxTvmId as BlackboxClientId
from tvmauth.mock import TvmClientPatcher, MockedTvmClient


@pytest.fixture
def tvm_aio_qloud(test_vcr, monkeypatch):
    from tvm2.aio.daemon_tvm2 import TVM2Daemon
    TVM2Daemon._instance = None
    monkeypatch.setenv('QLOUD_TVM_TOKEN', 'test_token')
    return TVM2Daemon(
        client_id='28',
        blackbox_client=BlackboxClientId.Test,
        retries=0,
    )


@pytest.fixture
def tvm_aio_deploy(test_vcr, monkeypatch):
    from tvm2.aio.daemon_tvm2 import TVM2Daemon
    TVM2Daemon._instance = None
    monkeypatch.setenv('DEPLOY_BOX_ID', 'backend')
    monkeypatch.setenv('DEPLOY_TVM_TOOL_URL', 'http://localhost:44')
    monkeypatch.setenv('TVMTOOL_LOCAL_AUTHTOKEN', 'deploy_test_token')

    return TVM2Daemon(
        client_id='28',
        blackbox_client=BlackboxClientId.Test,
        retries=0,
    )


@pytest.fixture
def tvm_aio_threaded(test_vcr):
    from tvm2.aio.thread_tvm2 import TVM2
    TVM2._instance = None
    with TvmClientPatcher(MockedTvmClient(self_tvm_id=28)):
        return TVM2(
            client_id='28',
            secret='GRMJrKnj4fOVnvOqe-WyD1',
            blackbox_client=BlackboxClientId.Test,
            destinations=['28'],
            retries=0,
        )
