import datetime

from unittest.mock import Mock

from tools.routing_bingo.solomon import SolomonClient, SolomonResult, SolomonProberResultVector, SolomonProberResult, \
    ProberStatus

RESULT = {
    'vector': [
        # matrix_target = target-local
        {
            'timeseries': {
                'alias': '', 'kind': 'DGAUGE', 'type': 'DGAUGE',
                'labels': {'cluster': 'testing', 'cluster_slug': 'rb-sg-http', 'matrix_target': 'target-local',
                           'metric': 'success', 'service': 'metrics', 'matrix_family': 'inet',
                           'host': 'agent.ru-central1-a',
                           'prober_slug': 'rb-sg-http', 'project': 'cloud_mr_prober'},
                'timestamps': [1647526618000, 1647526648000, 1647526678000],
                'values': [1.0, 1.0, 1.0]
            }
        },
        {
            'timeseries': {
                'alias': '', 'kind': 'DGAUGE', 'type': 'DGAUGE',
                'labels': {'cluster': 'testing', 'cluster_slug': 'rb-sg-http', 'matrix_target': 'target-local',
                           'metric': 'fail', 'service': 'metrics', 'matrix_family': 'inet',
                           'host': 'agent.ru-central1-a',
                           'prober_slug': 'rb-sg-http', 'project': 'cloud_mr_prober'},
                'timestamps': [1647526619000, 1647526649000, 1647526679000],
                'values': [0.0, 0.0, 0.0]
            }
        },

        # matrix_target = target-remote
        {
            'timeseries': {
                'alias': '', 'kind': 'DGAUGE', 'type': 'DGAUGE',
                'labels': {'cluster': 'testing', 'cluster_slug': 'rb-sg-http', 'matrix_target': 'target-remote',
                           'metric': 'success', 'service': 'metrics', 'matrix_family': 'inet',
                           'host': 'agent.ru-central1-a',
                           'prober_slug': 'rb-sg-http', 'project': 'cloud_mr_prober'},
                'timestamps': [1647526619000, 1647526619000, 1647526619000],
                'values': [1.0, 0.0, 1.0]
            }
        },
        {
            'timeseries': {
                'alias': '', 'kind': 'DGAUGE', 'type': 'DGAUGE',
                'labels': {'cluster': 'testing', 'cluster_slug': 'rb-sg-http', 'matrix_target': 'target-remote',
                           'metric': 'fail', 'service': 'metrics', 'matrix_family': 'inet',
                           'host': 'agent.ru-central1-a',
                           'prober_slug': 'rb-sg-http', 'project': 'cloud_mr_prober'},
                'timestamps': [1647526619000, 1647526649000, 1647526679000],
                'values': [0.0, 1.0, 0.0]
            }
        },
    ]
}


def test_solomon_process_results():
    solomon = SolomonClient(None, None)
    solomon.get_last_metrics = Mock()
    solomon.get_last_metrics.return_value = SolomonResult.parse_obj(RESULT)

    time_frame = (1647526610.0, 1647526684.0)
    results = solomon.get_prober_results(time_frame, {}, ["rb-sg-http", "rb-sg-non-simple"], 3)
    solomon.get_last_metrics.assert_called_with(
        time_frame, {
            "prober_slug": "rb-sg-http|rb-sg-non-simple",
            "metric": "success|fail",
        }
    )

    assert results == [
        SolomonProberResultVector(
            prober_slug='rb-sg-http', params={'target': 'target-local', 'family': 'inet'},
            results=[SolomonProberResult(
                timestamp=datetime.datetime(2022, 3, 17, 14, 16, 58),
                status=ProberStatus.SUCCESS
            ),
                SolomonProberResult(
                    timestamp=datetime.datetime(2022, 3, 17, 14, 17, 28),
                    status=ProberStatus.SUCCESS
                ),
                SolomonProberResult(
                    timestamp=datetime.datetime(2022, 3, 17, 14, 17, 58),
                    status=ProberStatus.SUCCESS
                )]
        ),
        SolomonProberResultVector(
            prober_slug='rb-sg-http', params={'target': 'target-remote', 'family': 'inet'},
            results=[SolomonProberResult(
                timestamp=datetime.datetime(2022, 3, 17, 14, 16, 59),
                status=ProberStatus.SUCCESS
            ),
                SolomonProberResult(
                    timestamp=datetime.datetime(2022, 3, 17, 14, 17, 29),
                    status=ProberStatus.FAIL
                ),
                SolomonProberResult(
                    timestamp=datetime.datetime(2022, 3, 17, 14, 16, 59),
                    status=ProberStatus.SUCCESS
                )]
        )
    ]
