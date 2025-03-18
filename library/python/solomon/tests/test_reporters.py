# pylint: disable=protected-access
import datetime
import threading
import unittest
import furl

from mock import patch

import solomon


NOW = datetime.datetime(2016, 1, 1, 12, 0, 0)
SHARD_ID = {
    'project': 'test_project',
    'cluster': 'test_cluster',
    'service': 'test_service',
}


def CHECK_PUSH_URL(url):
    url = furl.furl(url)
    assert set(url.query.params.items()) >= set(SHARD_ID.items())


class BasePushReporterTestCaseMixin(object):

    def test_single_sensor_payload(self):

        client = self.init_reporter(**SHARD_ID)
        CHECK_PUSH_URL(client.push_url)

        with patch.object(client, '_get_current_time', return_value=NOW), \
                patch.object(client, '_push') as push_mock:

            client.set_value(
                'test_sensor',
                'test_value',
                labels={
                    'label1': 'lv1',
                    'label2': 'lv2',
                },
            )

            expected_payload = {
                'sensors': [
                    {
                        'ts': '2016-01-01T12:00:00Z',
                        'value': 'test_value',
                        'labels': {
                            'sensor': 'test_sensor',
                            'label1': 'lv1',
                            'label2': 'lv2',
                        },
                    },
                ],
            }

            push_mock.assert_called_once_with(expected_payload)

    def test_multiple_sensors_payload(self):

        client = self.init_reporter(**SHARD_ID)
        CHECK_PUSH_URL(client.push_url)

        with patch.object(client, '_get_current_time', return_value=NOW), \
                patch.object(client, '_push') as push_mock:

            client.set_value(
                ['sensor1', 'sensor2'],
                ['value1', 'value2'],
                labels=[
                    {
                        's1l': 's1lv',
                    },
                    {
                        's2l1': 's2lv1',
                        's2l2': 's2lv2',
                    },
                ],
            )

            expected_payload = {
                'sensors': [
                    {
                        'ts': '2016-01-01T12:00:00Z',
                        'value': 'value1',
                        'labels': {
                            'sensor': 'sensor1',
                            's1l': 's1lv',
                        },
                    },
                    {
                        'ts': '2016-01-01T12:00:00Z',
                        'value': 'value2',
                        'labels': {
                            'sensor': 'sensor2',
                            's2l1': 's2lv1',
                            's2l2': 's2lv2',
                        },
                    },
                ],
            }

            push_mock.assert_called_once_with(expected_payload)


class PushApiReporterTestCase(BasePushReporterTestCaseMixin, unittest.TestCase):

    def init_reporter(self, project, cluster, service):
        return solomon.PushApiReporter(
            project=project,
            cluster=cluster,
            service=service,
        )


class ThrottledPushApiReporterTestCase(BasePushReporterTestCaseMixin, unittest.TestCase):

    def init_reporter(self, project, cluster, service):
        return solomon.ThrottledPushApiReporter(
            push_interval=1,
            project=project,
            cluster=cluster,
            service=service,
        )


class OnPushFailedTestCase(unittest.TestCase):

    class ErrorHandlingPushApiReporter(solomon.PushApiReporter):

        def __init__(self, *args, **kwargs):
            super(OnPushFailedTestCase.ErrorHandlingPushApiReporter, self).__init__(*args, **kwargs)
            self.is_error_handled = threading.Event()

        def on_push_failed(self, exc=None, response=None):
            self.is_error_handled.set()

    def test_push_failed(self):
        client = self.ErrorHandlingPushApiReporter(**SHARD_ID)
        self.assertFalse(client.is_error_handled.is_set())

        with patch.object(client._session, 'post', side_effect=RuntimeError):
            client.set_value('test', 0)

        client.is_error_handled.wait(5)
        self.assertTrue(client.is_error_handled.is_set())


class SysmondReporterTestCase(unittest.TestCase):

    def test_final_params(self):
        client = solomon.SysmondReporter(service_name='test')
        with patch.object(client, '_make_request') as make_request_mock:
            client.send_user_stat('test_sensor', 'test_value', labels={"test_label": "test_label_value"})
            expected_params = {
                'service': 'test',
                'sensor': 'test_sensor',
                'value': 'test_value',
                'label.test_label': 'test_label_value',
            }
            make_request_mock.assert_called_once_with(expected_params)
