import os
import time
from collections import namedtuple
from contextlib import contextmanager

import pytest

import mdb_salt_returner
import mock

mdb_salt_returner.__grains__ = {}
DEPLOY_HOST = 'deploy.test'
TARGET = 'mdb_salt_returner.'
MINION_ID = 'test.minion'
JOBS_URL = 'https://deploy.test/v1/minions/test.minion/jobs/jid-1/results'
JOB_RESULT = {"jid": "jid-1", "should": "works", "fun": "test.ping"}


class Test_send_return:
    @pytest.mark.parametrize(
        'bad_return',
        [
            {'return-without-jid': 'is-bad'},
            None,
            [{'non-dict-return': 'bad'}],
        ],
    )
    def test_bad_return(self, bad_return):
        with pytest.raises(mdb_salt_returner.MalformedReturn):
            mdb_salt_returner.send_return(bad_return)

    @contextmanager
    def minion_id_mock(self):
        with mock.patch.dict(mdb_salt_returner.__grains__, {'id': 'test.minion'}):
            yield

    @contextmanager
    def deploy_host_mock(self, return_value=None, side_effect=None):
        if side_effect is None and return_value is None:
            return_value = DEPLOY_HOST
        with mock.patch('mdb_salt_returner._get_deploy_api_host', return_value=return_value, side_effect=side_effect):
            yield

    def test_does_not_do_sent_if_minion_id_is_unknown(self):
        with self.deploy_host_mock():
            assert not mdb_salt_returner.send_return(JOB_RESULT)

    def test_does_not_do_sent_deploy_host_is_unknown(self):
        with self.minion_id_mock():
            with self.deploy_host_mock(side_effect=mdb_salt_returner.DeployConfigurationError):
                assert not mdb_salt_returner.send_return(JOB_RESULT)

    @contextmanager
    def configured_deploy(self):
        with self.minion_id_mock(), self.deploy_host_mock():
            yield

    def test_success_send(self, requests_mock):
        requests_mock.post(JOBS_URL)
        with self.configured_deploy():
            assert mdb_salt_returner.send_return(JOB_RESULT)

    def test_send_do_retries_on_service_errors(self, requests_mock):
        requests_mock.register_uri(
            'POST',
            JOBS_URL,
            [
                {
                    'text': 'Internal Server Error',
                    'status_code': 500,
                },
                {
                    'text': 'Payment Required',
                    'status_code': 402,
                },
                {
                    'text': '{"status": "ok"}',
                    'status_code': 200,
                },
            ],
        )
        with self.configured_deploy():
            with mock.patch('mdb_salt_returner.time.sleep') as sleep_mock:
                assert mdb_salt_returner.send_return(JOB_RESULT)
                assert sleep_mock.call_count == 2

    def test_send_do_retries_on_io_errors(self, requests_mock):
        requests_mock.register_uri('POST', JOBS_URL, exc=IOError)
        with self.configured_deploy():
            with mock.patch('mdb_salt_returner.time.sleep'):
                assert not mdb_salt_returner.send_return(JOB_RESULT)


ReturnerMocks = namedtuple('ReturnerMocks', ['get_deploy_version', 'send_return', 'store_return'])


@pytest.fixture
def returner_mocks():
    with mock.patch('mdb_salt_returner._get_deploy_version') as get_deploy_version:
        with mock.patch('mdb_salt_returner.send_return') as send_return:
            with mock.patch('mdb_salt_returner._store_return') as store_return:
                yield ReturnerMocks(get_deploy_version, send_return, store_return)


class Test_returner:
    def test_do_nothing_for_deploy_v1(self, returner_mocks):
        returner_mocks.get_deploy_version.return_value = 1

        mdb_salt_returner.returner({"jid": "1"})

        returner_mocks.send_return.assert_not_called()
        returner_mocks.store_return.assert_not_called()

    def test_do_not_store_when_send_success(self, returner_mocks):
        returner_mocks.get_deploy_version.return_value = 2
        returner_mocks.send_return.return_value = True

        mdb_salt_returner.returner({"jid": "1"})

        returner_mocks.store_return.assert_not_called()

    def test_do_store_when_return_fails(self, returner_mocks):
        returner_mocks.get_deploy_version.return_value = 2
        returner_mocks.send_return.return_value = False

        mdb_salt_returner.returner({"jid": "1"})

        returner_mocks.store_return.assert_called_once_with({"jid": "1"})

    def test_do_store_when_return_fails_with_exception(self, returner_mocks):
        returner_mocks.get_deploy_version.return_value = 2
        returner_mocks.send_return.side_effect = RuntimeError

        mdb_salt_returner.returner({"jid": "1"})

        returner_mocks.store_return.assert_called_once_with({"jid": "1"})


StoredReturnsMocks = namedtuple(
    'StoredReturnsMocks',
    ['os_path_exists', 'os_listdir', 'os_stat', 'open', 'json_load', 'send_return', 'os_unlink', 'os_utime'],
)


@pytest.fixture
def stored_returns_mocks():
    with mock.patch('mdb_salt_returner.os.path.exists') as os_path_exists:
        with mock.patch('mdb_salt_returner.os.listdir') as os_listdir:
            with mock.patch('mdb_salt_returner.os.stat') as os_stat:
                with mock.patch('mdb_salt_returner.open') as patched_open:
                    with mock.patch('mdb_salt_returner.json.load') as json_load:
                        with mock.patch('mdb_salt_returner.send_return') as send_return:
                            with mock.patch('mdb_salt_returner.os.unlink') as os_unlink:
                                with mock.patch('mdb_salt_returner.os.utime') as os_utime:
                                    yield StoredReturnsMocks(
                                        os_path_exists,
                                        os_listdir,
                                        os_stat,
                                        patched_open,
                                        json_load,
                                        send_return,
                                        os_unlink,
                                        os_utime,
                                    )


class Test_stored_returns:
    def test_do_nothing_on_nonexistent_store_path(self, stored_returns_mocks):
        stored_returns_mocks.os_path_exists.return_value = False

        mdb_salt_returner.send_stored_returns()

        stored_returns_mocks.os_listdir.assert_not_called()

    def test_skip_invalid_names(self, stored_returns_mocks):
        stored_returns_mocks.os_listdir.return_value = ['invalid_filename', 'invalid_filename_with.json']

        mdb_salt_returner.send_stored_returns()

        stored_returns_mocks.os_stat.assert_not_called()

    def test_too_recent_not_sent(self, stored_returns_mocks):
        stored_returns_mocks.os_listdir.return_value = ['test__1.json']

        stored_returns_mocks.os_stat.return_value.st_ctime = time.time()

        mdb_salt_returner.send_stored_returns()

        stored_returns_mocks.open.assert_not_called()

    def test_backoff_not_sent(self, stored_returns_mocks):
        stored_returns_mocks.os_listdir.return_value = ['test__1.json']

        stored_returns_mocks.os_stat.return_value.st_ctime = time.time() - 120
        stored_returns_mocks.os_stat.return_value.st_mtime = time.time()

        mdb_salt_returner.send_stored_returns()

        stored_returns_mocks.open.assert_not_called()

    def test_mtime_updated_on_error(self, stored_returns_mocks):
        stored_returns_mocks.os_listdir.return_value = ['test__1.json']

        stored_returns_mocks.os_stat.return_value.st_ctime = time.time() - 3600
        stored_returns_mocks.os_stat.return_value.st_mtime = time.time() - 3300

        stored_returns_mocks.json_load.return_value = {'test': 1}

        stored_returns_mocks.send_return.side_effect = RuntimeError

        mdb_salt_returner.send_stored_returns()

        stored_returns_mocks.os_unlink.assert_not_called()

        stored_returns_mocks.os_utime.assert_called_once_with(
            os.path.join(mdb_salt_returner.STORE_PATH, 'test__1.json'), None
        )

    def test_unparseable_files_deleted(self, stored_returns_mocks):
        stored_returns_mocks.os_listdir.return_value = ['test__1.json']

        stored_returns_mocks.os_stat.return_value.st_ctime = time.time() - 3600
        stored_returns_mocks.os_stat.return_value.st_mtime = time.time() - 3300

        stored_returns_mocks.json_load.side_effect = RuntimeError

        mdb_salt_returner.send_stored_returns()

        stored_returns_mocks.send_return.assert_not_called()

        stored_returns_mocks.os_unlink.assert_called_once_with(
            os.path.join(mdb_salt_returner.STORE_PATH, 'test__1.json')
        )

    def test_sent_ok(self, stored_returns_mocks):
        stored_returns_mocks.os_listdir.return_value = ['test__1.json']

        stored_returns_mocks.os_stat.return_value.st_ctime = time.time() - 3600
        stored_returns_mocks.os_stat.return_value.st_mtime = time.time() - 3300

        stored_returns_mocks.json_load.return_value = {'test': 1}

        mdb_salt_returner.send_stored_returns()

        stored_returns_mocks.send_return.assert_called_once_with({'test': 1}, minion_id='test')

        stored_returns_mocks.os_unlink.assert_called_once_with(
            os.path.join(mdb_salt_returner.STORE_PATH, 'test__1.json')
        )
