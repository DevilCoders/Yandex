from cloud.mdb.dbaas_worker.internal.providers.juggler import find_workers_downtime


def test_find_workers_downtime_in_empty_downtimes_list():
    assert find_workers_downtime({'items': []}, 'xdb.yt', '') is None


def test_find_workers_downtime_for_existed_worker_downtime():
    assert (
        find_workers_downtime(
            {
                'items': [
                    {
                        'filters': [
                            {
                                'host': 'xdb.yt',
                                'service': '',
                                'instance': '',
                                'namespace': '',
                                'tags': [],
                                'project': '',
                            }
                        ],
                        'start_time': 1632216428.0,
                        'end_time': 1632220028.0,
                        'description': 'dbaas-worker',
                        'user': 'robot-pgaas-deploy',
                        'downtime_id': '6149a56cb4c6fbc3c48972ec',
                        'source': '',
                        'startrek_ticket': '',
                        'project': '',
                        'warnings': [],
                    }
                ]
            },
            'xdb.yt',
            '',
        )
        == '6149a56cb4c6fbc3c48972ec'
    )


def test_find_workers_downtime_in_non_workers_downtimes():
    assert (
        find_workers_downtime(
            {
                'items': [
                    {
                        'filters': [
                            {
                                'host': 'xdb.yt',
                                'service': '',
                                'instance': '',
                                'namespace': '',
                                'tags': [],
                                'project': '',
                            }
                        ],
                        'start_time': 1632216428.0,
                        'end_time': 1632220028.0,
                        'description': 'mdb-cms',
                        'user': 'robot-pgaas-deploy',
                        'downtime_id': '6149a56cb4c6fbc3c48972ec',
                        'source': '',
                        'startrek_ticket': '',
                        'project': '',
                        'warnings': [],
                    }
                ]
            },
            'xdb.yt',
            '',
        )
        is None
    )
